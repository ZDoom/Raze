// Note: Duke's handling is dumb enough to make it impossible for other actors than the predefined projectile type to be used as projectile - 
// even if it is given the right statnum the projectile code won't get called for it.
// So even in the future any projectile needs to inherit from this to gain the needed feature support.

class DukeProjectile : DukeActor
{
	default
	{
		statnum STAT_PROJECTILE;
	}
	
	Vector3 oldpos;		// holds the position before the current move
	double velx, vely;	// holds the actual velocity for the current move. This can differ from the actor's internal values.
	Sound SpawnSound;
	
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
		targ.checkhitsprite(self);
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
		return self.weaponhitplayer(targ);
	}

	virtual bool weaponhitwall(walltype wal)
	{
		if (self.actorflag2(SFLAG2_MIRRORREFLECT) && dlevel.isMirror(wal))
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

			if (self.actorflag2(SFLAG2_REFLECTIVE))
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
		let oldpos = self.pos;

		int p = -1;

		if (self.actorflag2(SFLAG2_UNDERWATERSLOWDOWN) && self.sector.lotag == ST_2_UNDERWATER)
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


