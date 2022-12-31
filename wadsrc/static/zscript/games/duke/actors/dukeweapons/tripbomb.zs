
class DukeTripBomb : DukeActor
{
	enum EMode
	{
		// Control flags for WW2GI weapons.
		TRIPBOMB_TRIPWIRE = 1,
		TRIPBOMB_TIMER = 2
	};


	default
	{
		pic "TRIPBOMB";
		Strength TRIPBOMB_STRENGTH;
		strength 100;
		+CHECKSLEEP;
		+HITRADIUS_FORCEEFFECT;
		+MOVEFTA_MAKESTANDABLE;
		+SE24_NOCARRY;
		+DONTANIMATE;
		+NOFALLER;
		+BLOCK_TRIPBOMB;
		+NOFLOORPAL;

		+NOTELEPORT;
	}
	
	override void Initialize()
	{
		self.scale = (0.0625, 0.078125);
		ud.bomb_tag = (ud.bomb_tag + 1) & 32767;
		self.hitag = ud.bomb_tag;
		self.detail = TRIPBOMB_TRIPWIRE;
	}
	
	override void Tick()
	{
		int j;
		double x;
		
		if (self.statnum != STAT_STANDABLE)
		{
			return;
		}

		int lTripBombControl = self.detail;
		if (lTripBombControl & TRIPBOMB_TIMER)
		{
			// we're on a timer....
			if (self.extra >= 0)
			{
				self.extra--;
				if (self.extra == 0)
				{
					self.temp_data[2] = 16;
					self.PlayActorSound("LASERTRIP_ARMING");
				}
			}
		}
		if (self.temp_data[2] > 0)
		{
			self.temp_data[2]--;
			if (self.temp_data[2] == 8)
			{
				self.PlayActorSound("LASERTRIP_EXPLODE");
				for (j = 0; j < 5; j++) self.RandomScrap();
				int ex = self.extra;
				self.hitradius(gs.tripbombblastradius, ex >> 2, ex >> 1, ex - (ex >> 2), ex);

				let spawned = self.spawn("DukeExplosion2");
				if (spawned)
				{
					spawned.angle = self.angle;
					spawned.vel.X = 348 / 16.;
					spawned.DoMove(CLIPMASK0);
				}

				DukeStatIterator it;
				for(let a1 = it.First(STAT_MISC); a1; a1 = it.Next())
				{
					if (a1 is "DukeLaserLine" && self.hitag == a1.hitag)
						a1.scale = (0, 0);
				}
				self.Destroy();
			}
			return;
		}
		else
		{
			let ex = self.extra;
			self.extra = 1;
			let ang = self.angle;
			j = self.ifhitbyweapon();
			if (j >= 0)
			{ 
				self.temp_data[2] = 16; 
			}
			self.extra = ex;
			self.angle = ang;
		}

		if (self.counter < 32)
		{
			DukePlayer p;
			double x;
			[p, x] = self.findplayer();
			if (x > 48) self.counter++;
			else if (self.counter > 16) self.counter++;
		}
		if (self.counter == 32)
		{
			let ang = self.angle;
			self.angle = self.temp_angle;

			self.temp_pos = self.pos;
			self.pos += self.temp_angle.ToVector() * 2;
			self.pos.Z -= 3;

			// Laser fix from EDuke32.
			let oldSect = self.sector;
			let curSect = self.sector;

			curSect = Raze.updatesector(self.pos.XY, curSect, 128);
			self.ChangeSector(curSect);

			DukeActor hit;
			[x, hit] = self.hitasprite();

			self.temp_pos2.X = x;

			self.angle = ang;

			if (lTripBombControl & TRIPBOMB_TRIPWIRE)
			{
				// we're on a trip wire
				while (x > 0)
				{
					let spawned = self.spawn("DukeLaserLine");// LASERLINE);
					if (spawned)
					{
						spawned.SetPosition(spawned.pos);
						spawned.hitag = self.hitag;
						spawned.temp_pos.Z = spawned.pos.Z; // doesn't look to be used anywhere...

						if (x < 64)
						{
							spawned.scale.X = (x * (REPEAT_SCALE / 2));
							break;
						}
						x -= 64;

						self.pos += self.temp_angle.ToVector() * 64;
						cursect = Raze.updatesector(self.pos.XY, curSect, 128);

						if (curSect == nullptr)
							break;

						self.ChangeSector(curSect);

						// this is a hack to work around the laser line sprite's art tile offset
						spawned.ChangeSector(curSect);
					}
				}
			}

			self.counter++;
			self.pos = self.temp_pos;
			self.ChangeSector(oldSect);
			self.temp_data[3] = 0;
			if (hit && lTripBombControl & TRIPBOMB_TRIPWIRE)
			{
				self.temp_data[2] = 13;
				self.PlayActorSound("LASERTRIP_ARMING");
			}
			else self.temp_data[2] = 0;
		}
		if (self.counter == 33)
		{
			self.temp_data[1]++;


			self.temp_pos.XY = self.pos.XY;
			self.pos += self.temp_angle.ToVector() * 2;
			self.pos.Z -= 3;
			self.SetPosition(self.pos);

			x = self.hitasprite();

			self.pos.XY = self.temp_pos.XY;
			self.pos.Z += 3;
			self.SetPosition( self.pos);

			if (self.temp_pos2.X != x && lTripBombControl & TRIPBOMB_TRIPWIRE)
			{
				self.temp_data[2] = 13;
				self.PlayActorSound("LASERTRIP_ARMING");
			}
		}
	}
}


// the map-spawned and player-spawned trip bombs are different so let's spawn two distinct types for them.
class DukeTripBombPlaced : DukeTripBomb
{
	override void Initialize()
	{
		Super.Initialize();

		self.ownerActor = self;
		self.vel.X = 1;
		self.DoMove(CLIPMASK0);
		self.counter = 17;
		self.temp_data[2] = 0;
		self.temp_angle = self.angle;
		self.ChangeStat(STAT_ZOMBIEACTOR);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DukeLaserLine : DukeActor
{
	default
	{
		pic "LASERLINE";
		+FULLBRIGHT;
		+NOROTATEWITHSECTOR;
		+SHOWWALLSPRITEONMAP;
		+SE24_NOCARRY;
		+DONTANIMATE;
		+NOTELEPORT;
		+NOFLOORPAL;
	}

	override void Initialize()
	{
		self.scale = (0.5, 0.09375);

		if (gs.lasermode == 1)
			self.cstat = CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_TRANSLUCENT;
		else if (gs.lasermode == 0 || gs.lasermode == 2)
			self.cstat = CSTAT_SPRITE_ALIGNMENT_WALL;
		else
		{
			self.scale = (0, 0);
		}

		let owner = self.ownerActor;
		if (owner) self.angle = owner.temp_angle + 90;
		self.ChangeStat(STAT_MISC);
	}

	override bool animate(tspritetype t)
	{
		let OwnerAc = self.ownerActor;
		if (!OwnerAc) return true;
		if (t.sector.lotag == 2) t.pal = 8;
		t.pos.Z = OwnerAc.pos.Z - 3;
		if (gs.lasermode == 2 && Duke.GetViewPlayer().heat_on == 0)
			t.scale.Y = 0;
		return true;
	}
}

class DukeHandHoldingLaser : DukeActor
{
	default
	{
		pic "HANDHOLDINGLASER";
	}

	override bool ShootThis(DukeActor shooter, DukePlayer p, Vector3 pos, double ang) const
	{
		let sectp = shooter.sector;
		double vel = 1024., zvel;
		int j;
		HitInfo hit;

		if (p != null)
			[vel, zvel] = Raze.setFreeAimVelocity(vel, zvel, p.getPitchWithView(), 16.);
		else zvel = 0;

		Raze.hitscan(pos, sectp, (ang.ToVector() * vel, zvel * 64), hit, CLIPMASK1);

		j = 0;
		if (hit.hitActor) return true;

		if (hit.hitWall && hit.hitSector)
		{
			if ((hit.hitpos.XY - pos.XY).LengthSquared() < 18.125 * 18.125)
			{
				if (hit.hitWall.twoSided())
				{
					if (hit.hitWall.nextSectorp().lotag <= 2 && hit.hitSector.lotag <= 2)
						j = 1;
				}
				else if (hit.hitSector.lotag <= 2)
					j = 1;
			}

			if (j == 1)
			{
				let bomb = dlevel.SpawnActor(hit.hitSector, hit.hitpos, "DukeTripBomb", -16, (0.0625, 0.078125), ang, 0., 0., shooter, STAT_STANDABLE);
				if (!bomb) return true;

				if (gs.TripBombControl & DukeTripBomb.TRIPBOMB_TIMER)
				{
					// set timer.  blows up when at zero....
					bomb.extra = gs.stickybomb_lifetime + ((random(0, 65535) * gs.stickybomb_lifetime_var) >> 14) - gs.stickybomb_lifetime_var;
					bomb.detail = DukeTripBomb.TRIPBOMB_TIMER;
				}
				else
					bomb.detail = DukeTripBomb.TRIPBOMB_TRIPWIRE;	// this also covers the originally undefined case of tripbombcontrol == 0.

				// this originally used the sprite index as tag to link the laser segments.
				// This value is never used again to reference an shooter by index. Decouple this for robustness.
				ud.bomb_tag = (ud.bomb_tag + 1) & 32767;
				bomb.hitag = ud.bomb_tag;
				bomb.PlayActorSound("LASERTRIP_ONWALL");
				bomb.vel.X = -1.25;
				bomb.DoMove(CLIPMASK0);
				bomb.cstat = CSTAT_SPRITE_ALIGNMENT_WALL;
				let delta = -hit.hitWall.delta();
				bomb.Angle = delta.Angle() - 90;
				bomb.temp_angle = bomb.Angle;

				if (p)
					p.ammo_amount[DukeWpn.TRIPBOMB_WEAPON]--; // this should be elsewhere.
			}
		}
		return true;
	}
}
