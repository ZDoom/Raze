class DukeFlammable : DukeActor
{
	default
	{
		statnum STAT_STANDABLE;
		+HITRADIUS_FLAG1;
	}

	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_BLOCK_ALL; // Make it hitable
		self.extra = 1;
	}
	
	override void Tick()
	{
		if (self.temp_data[0] == 1)
		{
			self.temp_data[1]++;
			if ((self.temp_data[1] & 3) > 0) return;

			if (self.actorflag1(SFLAG_FLAMMABLEPOOLEFFECT) && self.temp_data[1] == 32)
			{
				self.cstat = 0;
				let spawned = self.spawn("DukeBloodPool");
				if (spawned) 
				{
					spawned.pal = 2;
					spawned.shade = 127;
					spawned.detail = 1;
				}
			}
			else
			{
				if (self.shade < 64) self.shade++;
				else
				{
					self.Destroy();
					return;
				}
			}

			double scale = self.scale.X - random(0, 7) * REPEAT_SCALE;
			if (scale < 0.15625)
			{
				self.Destroy();
				return;
			}
			self.scale.X = scale;

			scale = self.scale.Y - random(0, 7) * REPEAT_SCALE;
			if (scale < 0.0625)
			{
				self.Destroy();
				return;
			}
			self.scale.Y = scale;
		}
		if (self.actorflag1(SFLAG_FALLINGFLAMMABLE))
		{
			self.makeitfall();
			self.ceilingz = self.sector.ceilingz;
		}
	}
	
	override void onHit(DukeActor hitter)
	{
		if (hitter.actorflag1(SFLAG_INFLAME))
		{
			if (self.temp_data[0] == 0)
			{
				self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
				self.temp_data[0] = 1;
				self.spawn("DukeBurning");
			}
		}
	}
}

class DukeBox : DukeFlammable
{
	default
	{
		pic "BOX";
		+FALLINGFLAMMABLE;
	}
}

class DukeTree1 : DukeFlammable
{
	default
	{
		pic "TREE1";
		+NOFLOORFIRE;
	}
}

class DukeTree2 : DukeFlammable
{
	default
	{
		pic "TREE2";
		+NOFLOORFIRE;
	}
}

class DukeTire : DukeFlammable
{
	default
	{
		pic "TIRE";
		+FLAMMABLEPOOLEFFECT;
	}
}

class DukeCone : DukeFlammable
{
	default
	{
		pic "CONE";
	}
}

