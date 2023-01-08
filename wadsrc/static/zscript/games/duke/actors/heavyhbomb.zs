
class DukePipeBomb : DukeActor
{
	default
	{
		pic "HEAVYHBOMB";
		// do not add anything here!
	}

	override void Initialize()
	{
		// This is only for placed items, not for armed weapons!
		if (self.mapSpawned)
		{
			self.extra = gs.impact_damage;
			self.cstat |= CSTAT_SPRITE_BLOCK_ALL; // Make it hitable

			if (ud.multimode < 2 && self.pal != 0)
			{
				self.scale = (0, 0);
				self.ChangeStat(STAT_MISC);
				return;
			}
			self.ChangeStat(STAT_ZOMBIEACTOR);
			self.yint = 4;
			self.pal = 0;
			self.shade = -17;
			self.Scale = (0.140625, 0.140625);
		}
	}
		
	override void Tick()
	{
		if (self.statnum != STAT_ACTOR) return;
		let Owner = self.ownerActor;
		let sectp = self.sector;
		DukePlayer p;
		double xx;

		if ((self.cstat & CSTAT_SPRITE_INVISIBLE))
		{
			self.temp_data[2]--;
			if (self.temp_data[2] <= 0)
			{
				self.PlayActorSound("TELEPORTER");
				self.spawn("DukeTransporterStar");
				self.cstat = CSTAT_SPRITE_BLOCK_ALL;
			}
			return;
		}

		[p, xx] = self.findplayer();

		if (xx < 1220 / 16.) self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		else self.cstat |= CSTAT_SPRITE_BLOCK_ALL;

		if (self.temp_data[3] == 0)
		{
			int j = self.ifhitbyweapon();
			if (j >= 0)
			{
				self.temp_data[4] = 0;
				self.vel.X = 0;
				self.DetonateIt();
				return;
			}
		}

		self.makeitfall();

		// Feature check later needs to be map controlled, not game controlled.
		if (sectp.lotag != ST_1_ABOVE_WATER && (!Raze.isRRRA() || sectp.lotag != ST_160_FLOOR_TELEPORT) && self.pos.Z >= self.floorz - 1 && self.yint < 3)
		{
			if (self.yint > 0 || (self.yint == 0 && self.floorz == sectp.floorz))
			{
				self.PlayActorSound("PIPEBOMB_BOUNCE");
			}
			self.vel.Z = -(4 - self.yint);
			if (sectp.lotag == ST_2_UNDERWATER)
				self.vel.Z *= 0.25;
			self.yint++;
		}
		if (self.pos.Z < self.ceilingz)
		{
			self.pos.Z = self.ceilingz + 3;
			self.vel.Z = 0;
		}

		CollisionData coll;
		self.movesprite_ex((self.angle.ToVector() * self.vel.X, self.vel.Z), CLIPMASK0, coll);

		if (sectp.lotag == ST_1_ABOVE_WATER && self.vel.Z == 0)
		{
			self.pos.Z += 32;
			if (self.temp_data[5] == 0)
			{
				self.temp_data[5] = 1;
				self.spawn("DukeWaterSplash");
			}
		}
		else self.temp_data[5] = 0;

		if(self.vel.X > 0)
		{
			self.vel.X -= 5. / 16;
			if (sectp.lotag == ST_2_UNDERWATER)
				self.vel.X -= 10. / 16;

			if(self.vel.X < 0)
				self.vel.X = 0;
			if (int(self.vel.X * 16) & 8) self.cstat ^= CSTAT_SPRITE_XFLIP;
		}

		if (coll.type == kHitWall)
		{
			let wal = coll.hitWall();
			dlevel.checkhitwall(wal, self, self.pos);

			let k = wal.delta().Angle();
			self.angle = k * 2 - self.angle;
			self.vel.X *= 0.5;
		}
		
		bool bBoom = false;
		if (Owner && Owner.isPlayer())
		{
			if (Raze.isNamWW2GI())
			{
				self.extra--;
				if (self.extra <= 0)
					bBoom = true;
			}
			else bBoom = Owner.GetPlayer().hbomb_on == 0;
		}
		
		if (bBoom) self.DetonateIt();
		else self.pickupCheck(p);

		if (self.temp_data[0] < 8) self.temp_data[0]++;
	}

	void DetonateIt()
	{
		self.temp_data[4]++;

		if (self.temp_data[4] == 2)
		{
			int x = self.extra;
			int m = gs.pipebombblastradius;

			if (self.sector.lotag != 800 || !Raze.isRR()) // this line is RR only
			{
				self.hitradius(m, x >> 2, x >> 1, x - (x >> 2), x);
				self.spawn("DukeExplosion2");
				if (self.vel.Z == 0 && !Raze.isRR()) self.spawn("DukeExplosion2Bot"); // this line is Duke only
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
		}
	}
	
	protected bool doPickupCheck(DukePlayer p, int weapon1, int weapon2)
	{
		let xx = (p.actor.pos - self.pos).Sum();
		let Owner = self.ownerActor;
		// Duke
		if (xx < 788 / 16. && self.temp_data[0] > 7 && self.vel.X == 0)
			if (Raze.cansee(self.pos.plusZ(-8), self.sector, p.actor.pos.plusZ(p.actor.viewzoffset), p.cursector))
				if (p.ammo_amount[weapon1] < gs.max_ammo_amount[weapon1])
				{
					if (ud.coop >= 1 && Owner == self)
					{
						p.CheckWeapRec(self, false);
					}

					p.addammo(weapon1, 1);
					if (weapon2 >= 0) p.addammo(weapon2, 1);
					p.actor.PlayActorSound("PLAYER_GET");

					if (p.gotweapon[weapon1] == 0 || Owner == p.actor)
						p.addweapon(weapon1, true);

					if (!Owner || !Owner.isPlayer())
					{
						p.pals = Color(32, 0, 32, 0);
					}

					if (Owner != self || ud.respawn_items == 0)
					{
						if (Owner == self && ud.coop >= 1)
							return false;

						self.Destroy();
						return false;
					}
					else
					{
						self.temp_data[2] = gs.respawnitemtime;
						self.cstat = CSTAT_SPRITE_INVISIBLE;
						return true;
					}
				}
		return false;
	}

	virtual void pickupCheck(DukePlayer p)
	{
		if (doPickupCheck(p, DukeWpn.HANDBOMB_WEAPON, -1))
			self.spawn("DukeRespawnMarker");
	}
}
class RedneckDynamite : DukePipeBomb
{
	default
	{
		pic "DYNAMITE";
	}

	override void pickupCheck(DukePlayer p)
	{
		if (doPickupCheck(p, RRWpn.DYNAMITE_WEAPON, RRWpn.CROSSBOW_WEAPON))
			self.spawn("RedneckRespawnMarker");
	}
	
}

