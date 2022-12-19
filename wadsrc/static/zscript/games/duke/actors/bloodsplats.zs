class DukeBloodSplat1 : DukeActor
{
	default
	{
		Pic "BLOODSPLAT1";
	}

	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_ALIGNMENT_WALL;
		self.scale.X = 0.109375 + random(0, 7) * REPEAT_SCALE;
		self.scale.Y = 0.109375 + random(0, 7) * REPEAT_SCALE;
		self.pos.Z -= 16;
		if (!self.mapSpawned && self.ownerActor && self.ownerActor.pal == 6)
			self.pal = 6;
		self.insertspriteq();
		self.ChangeStat(STAT_MISC);
	}

	override void Tick()
	{
		if (self.temp_data[0] < 14 * 26)
		{
			let offset = frandom(0, 1);
			let lerp = 1. - (double(self.temp_data[0]) / (14 * 26));
			let zadj = (1. / 16.) * lerp;
			let sadj = (1. / 12.) * lerp * REPEAT_SCALE;
			self.pos.Z += zadj + offset * zadj;
			self.scale.Y += sadj + offset * sadj;
			self.temp_data[0]++;
		}
	}
	
	override bool Animate(tspritetype t)
	{
		if (t.pal == 6)
			t.shade = -127;
		else
			t.shade = 16;
		return true;
	}
	
	override bool shootthis(DukeActor shooter, DukePlayer p, Vector3 pos, double ang) const
	{
		let sectp = shooter.sector;
		double zvel;
		HitInfo hit;

		if (p != null) ang += frandom(-11.25, 11.25);
		else ang += 180 + frandom(-11.25, 11.25);
		
		zvel = 4 - frandom(0, 8);


		Raze.hitscan(pos, sectp, (ang.ToVector() * 1024, zvel * 64), hit, 1);

		let wal = hit.hitWall;
		if ( (pos.XY - hit.hitpos.XY).Length() >= 64)
			return true;
		if (wal == null)
			return true;
		if (wal.hitag != 0)
			return true;
		if (dlevel.wallflags(wal, 1) & Duke.TFLAG_NOBLOODSPLAT)
			return true;
		if ((wal.cstat & CSTAT_WALL_MASKED) != 0)
			return true;
		if (hit.hitSector.lotag != 0)
			return true;
		
		if (wal.twoSided())
		{
			if (wal.nextSectorp().lotag != 0 || (hit.hitSector.floorz - wal.nextSectorp().floorz) <= 16)
				return true;
			if (hit.hitSector.lotag != 0)
				return true;
			if (wal.nextWallp().hitag != 0)
				return true;
			
			DukeSectIterator it;
			for(let act2 = it.First(wal.nextSectorp()); act2; act2 = it.Next())
			{
				if (act2.statnum == STAT_EFFECTOR && act2.lotag == SE_13_EXPLOSIVE)
				{
					return true;
				}
			}
		}
		let spawned = shooter.spawn(self.getclassname());
		if (spawned)
		{
			spawned.vel.X = -0.75;
			spawned.Angle = wal.delta().Angle() - 90;
			spawned.pos = hit.hitpos;
			spawned.cstat |= randomXFlip();
			spawned.DoMove(CLIPMASK0);
			spawned.SetPosition(spawned.pos);
			spawned.cstat2 |= CSTAT2_SPRITE_DECAL;

			if (shooter.actorflag2(SFLAG2_GREENBLOOD))
				spawned.pal = 6;
		}
		return true;
	}
}

class DukeBloodSplat2 : DukeBloodSplat1
{
	default
	{
		Pic "BLOODSPLAT2";
	}
}

class DukeBloodSplat3 : DukeBloodSplat1
{
	default
	{
		Pic "BLOODSPLAT3";
	}
}

class DukeBloodSplat4 : DukeBloodSplat1
{
	default
	{
		Pic "BLOODSPLAT4";
	}
}
