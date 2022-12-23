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


	//---------------------------------------------------------------------------
	//
	// this is very, very messy. There's really no point trying to make this
	// moddable - that will require new shooting functions with better setup.
	//
	//---------------------------------------------------------------------------

	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		let sect = actor.sector;
		double vel, zvel;
		int scount;

		DukeActor act90 = nullptr;
		if (actor.extra >= 0) actor.shade = -96;

		scount = 1;
		vel = 40.25;

		DukeActor aimed = nullptr;

		if (p != null)
		{
			aimed = actor.aim(self);
			if (aimed)
			{
				if (self.GetClass() is 'RedneckChickenArrow')
				{
					if (aimed is 'RedneckHen')
						act90 = p.actor;
					else
						act90 = aimed;
				}
					double dal = ((aimed.scale.X * aimed.spriteHeight()) * 0.5) + 8;
					double dist = (p.actor.pos.XY - aimed.pos.XY).Length();
					zvel = ((aimed.pos.Z - pos.Z - dal) * vel) / dist;
					if (!(aimed.bSPECIALAUTOAIM))
						ang = (aimed.pos.XY - pos.XY).Angle();
				}
				else
					[vel, zvel] = Raze.setFreeAimVelocity(vel, zvel, p.getPitchWithView(), 40.5);
		}
		else
		{
			let j = actor.findplayer();
			ang = (j.actor.opos.XY - pos.XY).Angle();
			if (actor is 'DukeBoss3')
			{
				double zoffs = 32;
				if (Raze.isWorldTour()) // Twentieth Anniversary World Tour
					zoffs *= (actor.scale.Y * 0.8);
				pos.Z -= zoffs;
			}
			else if (actor is 'DukeBoss2')
			{
				vel += 8;
				double zoffs = 24;
				if (Raze.isWorldTour()) // Twentieth Anniversary World Tour
					zoffs *= (actor.scale.Y * 0.8);
				pos.Z += zoffs;
			}

			double dist = (j.actor.pos.XY - actor.pos.XY).Length();

			zvel = ((j.actor.opos.Z + j.actor.viewzoffset - pos.Z) * vel) / dist;

			if (actor.bBADGUY && (actor.hitag & face_player_smart))
				ang = actor.Angle + frandom(-22.5 / 8, 22.5 / 8);
			aimed = nullptr;
		}


		let offset = (ang + 61.171875).ToVector() * (1024. / 448.);
		let spawned = dlevel.SpawnActor(sect, pos.plusZ(-1) + offset, self.GetClass(), 0, (0.21875, 0.21875), ang, vel, zvel, actor, STAT_PROJECTILE);

		if (!spawned) return true;

		if (p != null)
		{
			let snd = self.spawnsound;
			if (snd > 0) spawned.PlayActorSound(snd);
		}

		spawned.seek_actor = act90;
		spawned.extra += random(0, 7);
		spawned.temp_actor = aimed;

		if (p == null)
		{
			// Setup shit is RPG only
			if (actor is 'RedneckHulk')
			{
				spawned.scale = (0.125, 0.125);
			}
			else if (actor is 'DukeBoss3')
			{
				Vector2 spawnofs = (sin(ang) * 4, cos(ang) * -4);
				let aoffs = 22.5 / 32.;

				if (random(0, 1))
				{
					spawnofs = -spawnofs;
					aoffs = -aoffs;
				}

				if (Raze.isWorldTour()) // Twentieth Anniversary World Tour
				{
					double siz = actor.scale.Y * 0.8;
					spawnofs *= siz;
					aoffs *= siz;
				}

				spawned.pos += spawnofs;
				spawned.Angle += aoffs;

				spawned.scale = (0.65625, 0.65625);
			}
			else if (actor is 'DukeBoss2')
			{
				Vector2 spawnofs = (sin(ang) * (1024. / 56.), cos(ang) * -(1024. / 56.));
				let aoffs = 22.5 / 16. - frandom(-45, 45);

				if (Raze.isWorldTour())  // Twentieth Anniversary World Tour
				{
					double siz = actor.scale.Y * 0.9143;
					spawnofs *= siz;
					aoffs *= siz;
				}

				spawned.pos += spawnofs;
				spawned.Angle += aoffs;

				spawned.scale = (0.375, 0.375);
			}
			else
			{
				spawned.scale = (0.46875, 0.46875);
				spawned.extra >>= 2;
			}
		}
		else if (p.curr_weapon == DukeWpn.DEVISTATOR_WEAPON)
		{
			spawned.extra >>= 2;
			spawned.Angle += frandom(-22.5 / 8, 22.5 / 8);
			spawned.vel.Z += frandom(-1, 1);

			if (p.hbomb_hold_delay)
			{
				Vector2 spawnofs = (sin(ang) * -(1024. / 644.), cos(ang) * (1024. / 644.));
				spawned.pos += spawnofs;
			}
			else
			{
				Vector2 spawnofs = (sin(ang) * 4, cos(ang) * -4);
				spawned.pos += spawnofs;
			}
			spawned.scale *= 0.5;
		}

		spawned.cstat = CSTAT_SPRITE_YCENTER;
		spawned.clipdist = 1;
		return true;
	}


}



