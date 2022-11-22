// This isn't used anywhere and looks broken anyway. Sprites are the animation of Shrinkspark. Only provided for completeness.
class DukeTongue : DukeActor
{
	default
	{
		statnum STAT_PROJECTILE;
		pic "TONGUE";
	}

	override void Initialize()
	{
		self.angle = self.ownerActor.angle;
		self.pos.Z -= gs.playerheight;
		self.vel.Z = 1 - frandom(0, 2);
		self.vel.X = 4 - frandom(0, 8);
	}

	override void Tick()
	{
		self.temp_data[0] = int(Raze.BobVal(self.temp_data[1]) * 32);
		self.temp_data[1] += 32;
		if (self.temp_data[1] > 2047 || statnum == STAT_MISC)
		{
			self.Destroy();
			return;
		}

		let Owner = self.ownerActor;
		if (!Owner)
		{
			self.Destroy();
		}

		self.angle = Owner.angle;
		self.pos = Owner.pos.plusZ(Owner.isPlayer() ? -34 : 0);

		for (int k = 0; k < self.temp_data[0]; k++)
		{
			let pos = self.pos + self.angle.ToVector() * 2 * k;
			pos.Z += k * self.vel.Z / 12;

			let q = dlevel.SpawnActor(self.sector, pos, "DukeTongue", -40 + (k << 1), (0.125, 0.125), 0, 0., 0., self, STAT_MISC);
			if (q)
			{
				q.ChangeStat(STAT_MISC);
				q.cstat = CSTAT_SPRITE_YCENTER;
				q.pal = 8;
			}
		}
		int k = self.temp_data[0];	// do not depend on the above loop counter.
		let pos = self.pos + self.angle.ToVector() * 2 * k;
		pos.Z += k * self.vel.Z / 12;
		let jaw = 'DukeInnerJaw';
		if (self.temp_data[1] > 512 && self.temp_data[1] < 1024) jaw = 'DukeInnerJaw1';
		let spawned = dlevel.SpawnActor(self.sector, pos, jaw, -40, (0.5, 0.5), 0, 0., 0., self, STAT_MISC);
		if (spawned)
		{
			spawned.cstat = CSTAT_SPRITE_YCENTER;
		}
	}
}

class DukeInnerJaw : DukeActor
{
	default
	{
		statnum STAT_MISC;
		pic "InnerJaw";
	}
	
	override void Tick()
	{
		double xx;
		DukePlayer p;
		[p, xx] = self.findplayer();
		if (xx < 32)
		{
			p.pals = Color(32, 32, 0, 0);
			p.actor.extra -= 4;
		}

		if (self.extra != 999)
			self.extra = 999;
		else
		{
			self.Destroy();
		}
	}
}

class DukeInnerJaw1 : DukeInnerJaw
{
	default
	{
		pic "InnerJaw1";
	}
}

