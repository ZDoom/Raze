class DukeFlamethrowerFlame : DukeActor
{
	default
	{
		pic "FLAMETHROWERFLAME";
	}
	
	override void Tick()
	{
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
				hitact.checkhitsprite(self);
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
