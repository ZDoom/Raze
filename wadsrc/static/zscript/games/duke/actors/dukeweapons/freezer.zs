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
	
	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		let sect = actor.sector;
		double vel, zvel;
		int scount;

		pos.Z += 3;
		if (actor.extra >= 0 && !self.bDONTLIGHTSHOOTER) actor.shade = -96;

		scount = 1;
		vel = 40.25;

		if (p != null)
		{
			let aimed = actor.aim(self);

			if (aimed)
			{
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

			double dist = (j.actor.pos.XY - actor.pos.XY).Length();
			zvel = ((j.actor.opos.Z + j.actor.viewzoffset - pos.Z) * vel) / dist;

			if (actor.bBADGUY && (actor.hitag & face_player_smart))
				ang = actor.Angle + frandom(-22.5 / 8, 22.5 / 8);
		}


		let offset = (ang + 61.171875).ToVector() * (1024. / 448.);
		let spawned = dlevel.SpawnActor(sect, pos.plusZ(-1) + offset, self.GetClass(), 0, (0.109375, 0.109375), ang, vel, zvel, actor, STAT_PROJECTILE);

		if (!spawned) return true;

		if (p != null)
		{
			let snd = self.spawnsound;
			if (snd > 0) spawned.PlayActorSound(snd);
		}

		spawned.extra += random(0, 7);
		spawned.yint = gs.numfreezebounces;
		spawned.vel.Z -= 0.25;
		spawned.cstat = CSTAT_SPRITE_YCENTER;
		spawned.clipdist = 10;
		return true;
	}

	
}


