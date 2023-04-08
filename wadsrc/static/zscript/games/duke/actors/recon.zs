class DukeRecon : DukeActor
{
	default
	{
		spriteset "RECON", "RECON2";
	}
	
	Sound AttackSnd;
	Sound PainSnd;
	Sound RoamSnd;
	int shift;
	class<DukeActor> spawntype;
	
	override void initialize()
	{
		self.temp_data[5] = 0;
		self.extra = 130;
		self.pal = 0;
		self.shade = -17;

	
		AttackSnd = "RECO_ATTACK";
		PainSnd = "RECO_PAIN";
		RoamSnd = "RECO_ROAM";
		shift =	4;
		SpawnType = "DukePigCop";
	}
	
	override void Tick()
	{
		let sectp = self.sector;
		double a;

		self.getglobalz();

		if (sectp.ceilingstat & CSTAT_SECTOR_SKY)
			self.shade += (sectp.ceilingshade - self.shade) >> 1;
		else self.shade += (sectp.floorshade - self.shade) >> 1;

		if (self.pos.Z < sectp.ceilingz + 32)
			self.pos.Z = sectp.ceilingz + 32;

		if (self.ifhitbyweapon() >= 0)
		{
			if (self.extra < 0 && self.temp_data[0] != -1)
			{
				self.temp_data[0] = -1;
				self.extra = 0;
			}
			if (painsnd >= 0) self.PlayActorSound(painsnd);
			self.RANDOMSCRAP();
		}

		if (self.temp_data[0] == -1)
		{
			self.pos.Z += 4;
			self.temp_data[2]++;
			if ((self.temp_data[2] & 3) == 0) self.spawn("DukeExplosion2");
			self.getglobalz();
			self.angle += 22.5 * 0.75;
			self.vel.X = 8;
			int j = self.DoMove(CLIPMASK0);
			if (j != 1 || self.pos.Z > self.floorz)
			{
				for (int l = 0; l < 16; l++)
					self.RANDOMSCRAP();
				self.PlayActorSound("LASERTRIP_EXPLODE");
				let spawned = self.spawn(spawntype);
				self.addkill();
				self.Destroy();
			}
			return;
		}
		else
		{
			if (self.pos.Z > self.floorz - 48)
				self.pos.Z = self.floorz - 48;
		}

		double xx;
		DukePlayer p;
		[p, xx] = self.findplayer();
		let pactor = p.actor;
		let Owner = self.ownerActor;

		// 3 = findplayerz, 4 = shoot

		if (self.temp_data[0] >= 4)
		{
			self.temp_data[2]++;
			if ((self.temp_data[2] & 15) == 0)
			{
				a = self.angle;
				self.angle = self.temp_angle;
				if (attacksnd >= 0) self.PlayActorSound(attacksnd);
				self.shoot("DukeFirelaser");
				self.angle = a;
			}
			if (self.temp_data[2] > (26 * 3) || !Raze.cansee(self.pos.plusZ(-16), self.sector, pactor.pos.plusZ(pactor.viewzoffset), p.cursector))
			{
				self.temp_data[0] = 0;
				self.temp_data[2] = 0;
			}
			else self.temp_angle +=
				deltaangle(self.temp_angle, (pactor.pos.XY - self.pos.XY).Angle()) / 3;
		}
		else if (self.temp_data[0] == 2 || self.temp_data[0] == 3)
		{
			if(self.vel.X > 0) self.vel.X -= 1;
			else self.vel.X = 0;

			if (self.temp_data[0] == 2)
			{
				double l = pactor.pos.Z + pactor.viewzoffset - self.pos.Z;
				if (abs(l) < 48) self.temp_data[0] = 3;
				else self.pos.Z += l < 0? -shift : shift; // The shift here differs between Duke and RR.
			}
			else
			{
				self.temp_data[2]++;
				if (self.temp_data[2] > (26 * 3) || !Raze.cansee(self.pos.plusZ(-16), self.sector, pactor.pos.plusZ(pactor.viewzoffset), p.cursector))
				{
					self.temp_data[0] = 1;
					self.temp_data[2] = 0;
				}
				else if ((self.temp_data[2] & 15) == 0 && attacksnd >= 0)
				{
					self.PlayActorSound(attacksnd);
					self.shoot("DukeFirelaser");
				}
			}
			self.angle += deltaangle(self.angle, (pactor.pos.XY - self.pos.XY).Angle()) * 0.25;
		}

		if (self.temp_data[0] != 2 && self.temp_data[0] != 3 && Owner)
		{
			double dist = (Owner.pos.XY - self.pos.XY).Length();
			if (dist <= 96)
			{
				a = self.angle;
				self.vel.X *= 0.5;
			}
			else a = (Owner.pos.XY - self.pos.XY).Angle();

			if (self.temp_data[0] == 1 || self.temp_data[0] == 4) // Found a locator and going with it
			{
				dist = (Owner.pos - self.pos).Length();

				if (dist <= 96) { if (self.temp_data[0] == 1) self.temp_data[0] = 0; else self.temp_data[0] = 5; }
				else
				{
					// Control speed here
					if (dist > 96) { if (self.vel.X < 16) self.vel.X += 2.; }
					else
					{
						if(self.vel.X > 0) self.vel.X -= 1;
						else self.vel.X = 0;
					}
				}

				if (self.temp_data[0] < 2) self.temp_data[2]++;

				if (xx < 384 && self.temp_data[0] < 2 && self.temp_data[2] > (26 * 4))
				{
					self.temp_data[0] = 2 + random(0, 1) * 2;
					self.temp_data[2] = 0;
					self.temp_angle = self.angle;
				}
			}

			if (self.temp_data[0] == 0 || self.temp_data[0] == 5)
			{
				if (self.temp_data[0] == 0)
					self.temp_data[0] = 1;
				else self.temp_data[0] = 4;
				let NewOwner = dlevel.LocateTheLocator(self.hitag, nullptr);
				if (!NewOwner)
				{
					self.hitag = self.temp_data[5];
					NewOwner = dlevel.LocateTheLocator(self.hitag, nullptr);
					if (!NewOwner)
					{
						self.Destroy();
						return;
					}
				}
				else self.hitag++;
				self.ownerActor = NewOwner;
			}

			let ang = deltaangle(self.angle, a);
			self.angle += ang * 0.125;

			if (self.pos.Z < Owner.pos.Z - 2)
				self.pos.Z += 2;
			else if (self.pos.Z > Owner.pos.Z + 2)
				self.pos.Z -= 2;
			else self.pos.Z = Owner.pos.Z; 
		}

		if (roamsnd >= 0 && !self.CheckSoundPlaying(roamsnd))
			self.PlayActorSound(roamsnd);

		self.DoMove(CLIPMASK0);
	}
	
	override bool Animate(tspritetype t)
	{
		t.SetSpritePic(self, abs(self.temp_data[3]) > 64);
		return true;
	}
	
	override void PlayFTASound()
	{
		self.PlayActorSound("RECO_RECOG");
	}

}

//---------------------------------------------------------------------------
//
// RR's UFOs use the same logic, but different setup.
//
//---------------------------------------------------------------------------

class RedneckUFO1 : DukeRecon
{
	default
	{
		Pic "UFO1_RR";
	}
	
	override void Initialize()
	{
		self.ChangeStat(STAT_ZOMBIEACTOR);
		
		RoamSnd = "UFOLET";
		shift =	1;
		SpawnType = "RedneckHen";
		self.Scale = (0.5, 0.5);
		self.Extra = 50;

		self.setClipDistFromTile();
	}

	override bool Animate(tspritetype t)
	{
		return true;
	}
	
	override void PlayFTASound()
	{
	}
}

class RedneckUFO2 : RedneckUFO1
{
	default
	{
		Pic "UFO2";
	}
	
	override void Initialize()
	{
		Super.Initialize();
		SpawnType = "RedneckCoot";
	}
}

class RedneckUFO3 : RedneckUFO1
{
	default
	{
		Pic "UFO3";
	}
	
	override void Initialize()
	{
		Super.Initialize();
		SpawnType = "RedneckCow";
	}
}

class RedneckUFO4 : RedneckUFO1
{
	default
	{
		Pic "UFO4";
	}
	
	override void Initialize()
	{
		Super.Initialize();
		SpawnType = "RedneckPig";
	}
}

class RedneckUFO5 : RedneckUFO1
{
	default
	{
		Pic "UFO5";
	}
	
	override void Initialize()
	{
		Super.Initialize();
		SpawnType = "RedneckBillyRay";
	}
}

class RedneckUFORRRA : RedneckUFO1
{
	default
	{
		Pic "UFO1_RRRA";
	}
	
	override void Initialize()
	{
		Super.Initialize();
		if (ud.ufospawnsminion) SpawnType = "RedneckMinion";
	}
}

class RedneckUfoSpawnerToggle : DukeActor
{
	default
	{
		statnum STAT_MISC;
	}
	override void Initialize()
	{
		//case RRTILE8192:
		self.scale = (0, 0);
		ud.ufospawnsminion = 1;
	}
}