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
	
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		pos.Z -= 2;
		shootprojectile1(actor, p, pos, ang, 52.5, 0);
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

class RedneckFirelaser : DukeFirelaser
{
	default
	{
		spriteset "FIRELASER", "FIRELASER2", "FIRELASER3", "FIRELASER4", "FIRELASER5", "FIRELASER6";
	}
	
	override bool animate(tspritetype tspr)
	{
		tspr.setSpritePic(self, ((PlayClock >> 2) % 6));
		return true;
	}
	
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		pos.Z -= 4;
		shootprojectile1(actor, p, pos, ang, 52.5, 0, 0.125);
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
	
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		pos.Z -= 10;
		shootprojectile1(actor, p, pos, ang, 292/16., 0);
		return true;
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
		+SPECIALINIT;
	}
	
	override void Initialize()
	{
		if (!bSIMPLEINIT)
		{
			// looks like this case is never used anywhere.
			self.cstat = CSTAT_SPRITE_YCENTER | self.randomXFlip();
			self.angle = self.ownerActor.angle;
			self.shade = -64;

			double c, f;
			[c, f] = self.sector.getSlopes(self.pos.XY);
			if (self.pos.Z > f - 12)
				self.pos.Z = f - 12;
		}
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

	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		pos.Z -= 10;
		let spawned = shootprojectile1(actor, p, pos, ang, 292/16., 0);
		if (spawned) 
		{
			spawned.shade = 0;
			// special hack case.
			if (actor.bSPECIALINIT)
			{
				let ovel = spawned.vel.X;
				spawned.vel.X = 64;
				spawned.DoMove(CLIPMASK0);
				spawned.vel.X = ovel;
				spawned.Angle += frandom(-22.5, 22.5);
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
	
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		if (actor.bSPAWNRABBITGUTS)
		{
			shootprojectile1(actor, p, pos, ang, 37.5, -20);
		}
		else
		{
			shootprojectile1(actor, p, pos, ang, 25, -10);
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
		if (Raze.tileflags(wal.walltexture) & Duke.TFLAG_NOCIRCLEREFLECT)
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