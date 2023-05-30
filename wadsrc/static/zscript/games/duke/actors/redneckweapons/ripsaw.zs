
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class RedneckSawBlade : DukeProjectile
{
	default
	{
		+CANHURTSHOOTER;
		spriteset "SAWBLADE", "SAWBLADE2", "SAWBLADE3",  "SAWBLADE4",  "SAWBLADE5",  "SAWBLADE6",  "SAWBLADE7",  "SAWBLADE8",
			"CHEERBLADE", "CHEERBLADE2", "CHEERBLADE3", "CHEERBLADE4";
		Strength THROWSAW_WEAPON_STRENGTH;
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

	//---------------------------------------------------------------------------
	//
	//
	//---------------------------------------------------------------------------

	override bool ShootThis(DukeActor actor, DukePlayer p, Vector3 pos, double ang) const
	{
		let sect = actor.sector;
		double vel, zvel;
		int scount;

		if (actor.extra >= 0) actor.shade = -96;

		scount = 1;
		vel = 40.25;

		DukeActor aimed = nullptr;

		if (p != null)
		{
			aimed = actor.aim(self);
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

		spawned.extra += random(0, 7);
		spawned.temp_actor = aimed;

		if (p == null)
		{
			spawned.scale = (0.46875, 0.46875);
			spawned.extra >>= 2;
		}

		spawned.cstat = CSTAT_SPRITE_YCENTER;
		spawned.clipdist = 1;
		return true;
	}

}


class RedneckCircleStuck : DukeActor
{
	default
	{
		pic "CIRCLESTUCK";
		Strength 0;
	}

}
