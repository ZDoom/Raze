
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
		// Note: The trip bomb has its health defined through CON! Value is 100. Con-based definitions will take precendence.
		health 100;
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
					self.PlayActorSound(DukeSnd.LASERTRIP_ARMING);
				}
			}
		}
		if (self.temp_data[2] > 0)
		{
			self.temp_data[2]--;
			if (self.temp_data[2] == 8)
			{
				self.PlayActorSound(Dukesnd.LASERTRIP_EXPLODE);
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

		if (self.temp_data[0] < 32)
		{
			DukePlayer p;
			double x;
			[p, x] = self.findplayer();
			if (x > 48) self.temp_data[0]++;
			else if (self.temp_data[0] > 16) self.temp_data[0]++;
		}
		if (self.temp_data[0] == 32)
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

			self.temp_data[0]++;
			self.pos = self.temp_pos;
			self.ChangeSector(oldSect);
			self.temp_data[3] = 0;
			if (hit && lTripBombControl & TRIPBOMB_TRIPWIRE)
			{
				self.temp_data[2] = 13;
				self.PlayActorSound(DukeSnd.LASERTRIP_ARMING);
			}
			else self.temp_data[2] = 0;
		}
		if (self.temp_data[0] == 33)
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
				self.PlayActorSound(DukeSnd.LASERTRIP_ARMING);
			}
		}
	}
}


// the map-spawned and player-spawned trip bombs are different so let's spawn two distinct types for them.
class DukeTripBombPlaced : DukeTripBomb
{
	override void Initialize()
	{
		if (self.lotag > ud.player_skill)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
			return;
		}
		Super.Initialize();

		self.ownerActor = self;
		self.vel.X = 1;
		self.DoMove(CLIPMASK0);
		self.temp_data[0] = 17;
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
		statnum STAT_MISC;
		pic "LASERLINE";
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
		t.shade = -127;
		return true;
	}


	
}

