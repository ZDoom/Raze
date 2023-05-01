
class DukeGrowSpark : DukeActor
{
	default
	{
		spriteset "GROWSPARK", "GROWSPARK1", "GROWSPARK2", "GROWSPARK3";
		Strength GROWSPARK_WEAPON_STRENGTH;
		+FULLBRIGHT;
		+NOFLOORPAL;
	}


	override bool ShootThis(DukeActor shooter, DukePlayer p, Vector3 pos, double ang) const
	{
		let sect = shooter.sector;
		double vel = 1024., zvel;
		int k;
		HitInfo hit;

		if (p != null)
		{
			let aimed = shooter.aim(self);
			if (aimed)
			{
				double dal = ((aimed.scale.Y * aimed.spriteHeight()) * 0.5) + aimed.sparkoffset; // originally used aimed.scale.X which is not correct.
				double dist = (p.actor.pos.XY - aimed.pos.XY).Length();
				zvel = ((aimed.pos.Z - pos.Z - dal) * 16) / dist;
				ang = (aimed.pos.XY - pos.XY).Angle();
			}
			else
			{
				ang += 22.5 / 8 - frandom(0, 22.5 / 4);
				[vel, zvel] = Raze.setFreeAimVelocity(vel, zvel, p.getPitchWithView(), 16.);
				zvel += 0.5 - frandom(0, 1);
			}

			pos.Z -= 2;
		}
		else
		{
			double x;
			DukePlayer j;
			[j, x] = self.findplayer();
			pos.Z -= 4;
			double dist = (j.actor.pos.XY - shooter.pos.XY).Length();
			zvel = ((j.actor.pos.Z + j.actor.viewzoffset - pos.Z) * 16) / dist;
			zvel += 0.5 - frandom(0, 1);
			ang += 22.5 / 4 - frandom(0, 22.5 / 2);
		}

		k = 0;

		//RESHOOTGROW:

		shooter.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		Raze.hitscan(pos, sect, (ang.ToVector() * vel, zvel * 64), hit, CLIPMASK1);

		shooter.cstat |= CSTAT_SPRITE_BLOCK_ALL;

		let spark = dlevel.SpawnActor(sect, hit.hitpos, "DukeGrowSpark", -16, (0.4375, 0.4375), ang, 0., 0., shooter, 1);
		if (!spark) return true;

		spark.pal = 2;
		spark.cstat |= CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
		spark.scale = (REPEAT_SCALE, REPEAT_SCALE);

		if (hit.hitWall == nullptr && hit.hitActor == nullptr && hit.hitSector != nullptr)
		{
			if (zvel < 0 && (hit.hitSector.ceilingstat & CSTAT_SECTOR_SKY) == 0)
				dlevel.checkhitceiling(hit.hitSector, spark);
		}
		else if (hit.hitActor != nullptr) DukeActor(hit.hitActor).OnHit(spark);
		else if (hit.hitWall != nullptr)
		{
			if (!Duke.isaccessswitch(hit.hitWall.walltexture))
			{
				dlevel.checkhitwall(hit.hitWall, spark, hit.hitpos);
			}
		}
		return true;
	}
	
	override bool animate(tspritetype t)
	{
		t.setSpritePic(self, (PlayClock >> 4) & 3);
		return true;
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.counter >= 18)
		{
			self.killit();
		}
		else
		{
			double dest = self.counter >= 9? 0 : 0.4375;
			for(int i = 0; i < 4; i++) self.actorsizeto(dest, dest);
		}
	}
	
}
