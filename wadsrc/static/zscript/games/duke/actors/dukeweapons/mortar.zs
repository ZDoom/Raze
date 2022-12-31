class DukeMortar : DukeActor
{
	default
	{
		pic "MORTER";
		Strength MORTER_WEAPON_STRENGTH;

		DukeMortar.ceilingdist 3;
		DukeMortar.spawnscale 0.5;
		+HITRADIUS_DONTHURTSHOOTER;
	}

	meta int ceilingdist;
	meta double spawnscale;
	property ceilingdist: ceilingdist;
	property spawnscale: spawnscale;
	

	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		let sect = actor.sector;
		if (actor.extra >= 0) actor.shade = -96;

		double x;
		DukePlayer p;
		[p, x] = actor.findplayer();
		let plActor = p.Actor;
		x = (plActor.pos.XY - actor.pos.XY).Length();

		double zvel = -x * 0.5;

		if (zvel < -8)
			zvel = -4;
		double vel = x / 16.;
		dlevel.SpawnActor(sect, pos.plusZ(-6) + ang.ToVector() * 4, GetClass(), -64, (self.spawnscale, self.spawnscale), ang, vel, zvel, actor, STAT_ACTOR);
		return true;
	}

	override void Tick()
	{
		let spawned = self.spawn("DukeFrameEffect");
		if (spawned) spawned.counter = 3;
		Common(1);
	}
	
	void Common(int itemmode)
	{
		let sectp = self.sector;
		DukePlayer p;
		double xx;
		
		[p,xx] = self.findplayer();

		if (xx < 1220 / 16.) self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		else self.cstat |= CSTAT_SPRITE_BLOCK_ALL;

		if (self.temp_data[3] == 0)
		{
			int j = self.ifhitbyweapon();
			if (j >= 0)
			{
				self.temp_data[3] = 1;
				self.temp_data[4] = 0;
				self.vel.X = 0;
				self.DetonateIt(itemmode);
				return;
			}
		}

		if (itemmode != 0)
		{
			self.makeitfall();

			// Feature check later needs to be map controlled, not game controlled.
			if (sectp.lotag != ST_1_ABOVE_WATER && (!(ud.mapflags & MFLAG_ALLSECTORTYPES) || sectp.lotag != ST_160_FLOOR_TELEPORT) && self.pos.Z >= self.floorz - 1 && self.yint < 3)
			{
				if (self.yint > 0 || (self.yint == 0 && self.floorz == sectp.floorz))
				{
					if (itemmode != 2)
						self.PlayActorSound("PIPEBOMB_BOUNCE");
					else if (self.temp_data[3] == 0)
					{
						self.temp_data[3] = 1;
						self.temp_data[4] = 0;
						self.DetonateIt(itemmode);
						return;
					}
				}
				self.vel.Z = -(4 - self.yint);
				if (sectp.lotag == 2)
					self.vel.Z *= 0.25;
				self.yint++;
			}
			if (itemmode != 2 && self.pos.Z < self.ceilingz + self.ceilingdist && (!Raze.isRR() || sectp.lotag != ST_2_UNDERWATER)) // underwater check only for RR
			{
				self.pos.Z = self.ceilingz + self.ceilingdist;
				self.vel.Z = 0;
			}
		}

		CollisionData coll;
		self.movesprite_ex((self.angle.ToVector() * self.vel.X, self.vel.Z), CLIPMASK0, coll);

		if (sectp.lotag == ST_1_ABOVE_WATER && self.vel.Z == 0)
		{
			self.pos.Z += 32;
			if (self.temp_data[0] == 0)
			{
				self.temp_data[0] = 1;
				self.spawn("DukeWaterSplash");
			}
		}
		else self.temp_data[0] = 0;

		if (self.temp_data[3] == 0 && (coll.type || xx < 844 / 16.))
		{
			self.temp_data[3] = 1;
			self.temp_data[4] = 0;
			self.vel.X = 0;
		}
		else if(self.vel.X > 0)
		{
			self.vel.X -= 5. / 16;
			if (sectp.lotag == ST_2_UNDERWATER)
				self.vel.X -= 10. / 16;

			if(self.vel.X < 0)
				self.vel.X = 0;
			//if (int(self.vel.X * 16) & 8) self.spr.cstat ^= CSTAT_SPRITE_XFLIP;
		}
		self.DetonateIt(itemmode);
	}

	void DetonateIt(int itemmode)
	{
		if (self.temp_data[3] == 1)
		{
			self.temp_data[4]++;

			DukePlayer p;
			double xx;
			
			[p,xx] = self.findplayer();

			if (self.temp_data[4] == 2)
			{
				int x = self.extra;
				int m = itemmode == 0? gs.bouncemineblastradius : gs.morterblastradius;

				if (self.sector.lotag != 800 || !Raze.isRR()) // this line is RR only
				{
					self.hitradius(m, x >> 2, x >> 1, x - (x >> 2), x);
					self.spawn("DukeExplosion2");
					if (self.vel.Z == 0 && !Raze.isRR()) self.spawn("DukeExplosion2Bot"); // this line is Duke only
					if (itemmode == 2) self.spawn("DukeBurning");
					self.PlayActorSound("PIPEBOMB_EXPLODE");
					for (x = 0; x < 8; x++)
						self.RANDOMSCRAP();
				}
			}

			if (self.scale.Y)
			{
				self.scale.Y = 0;
				return;
			}
			if (self.temp_data[4] > 20)
			{
				self.Destroy();
				return;
			}
			if (itemmode == 2)
			{
				self.spawn("DukeBurning");
				self.Destroy();
				return;
			}
		}
		if (self.counter < 8) self.counter++;
	}
	
}

class DukeBounceMine : DukeMortar
{
	default
	{
		pic "BOUNCEMINE";
		Strength BOUNCEMINE_WEAPON_STRENGTH;
	}
	
	override void Initialize()
	{
		// This is only for placed items, not for armed weapons!
		// Although this can be shot, it doesn't really work for that.
		if (self.mapSpawned)
		{
			self.ownerActor = self;
			self.extra = gs.impact_damage << 2;
			self.cstat |= CSTAT_SPRITE_BLOCK_ALL; // Make it hitable
			self.ChangeStat(STAT_ZOMBIEACTOR);
			self.shade = -127;
			self.Scale = (0.375, 0.375);
		}
	}
	
	override void Tick()
	{
		Common(0);
	}
}

class RedneckMortar : DukeMortar
{
	default
	{
		DukeMortar.ceilingdist 16;
	}
	
	override void Tick()
	{
		Common(1);
	}

}

class RedneckCheerBomb : DukeMortar
{
	default
	{
		spriteset "CHEERBOMB", "CHEERBOMB1", "CHEERBOMB2", "CHEERBOMB3";
		DukeMortar.ceilingdist 16;
		+NOFLOORPAL;
		DukeMortar.spawnscale 0.25;
	}
	
	override bool Animate(tspritetype t)
	{
		t.SetSpritePic(self, (PlayClock >> 4) & 3);
		return true;
	}
	
	override void Tick()
	{
		Common(2);
	}
}


