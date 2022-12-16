
class DukeOozFilter : DukeActor
{
	default
	{
		spriteset "OOZFILTER";
		+EXPLOSIVE;
		+BRIGHTEXPLODE;
		+DOUBLEDMGTHRUST;
		+BREAKMIRRORS;
		+GREENBLOOD;
	}
	
	override void Initialize()
	{
		self.shade = -16;
		if (self.scale.X <= 0.125)
		{
			self.cstat = CSTAT_SPRITE_INVISIBLE;
			self.scale = (0, 0);
		}
		else self.cstat = CSTAT_SPRITE_BLOCK_ALL;
		self.extra = gs.impact_damage << 2;
		self.ownerActor = self;
		self.ChangeStat(STAT_STANDABLE);
	}
	
	override void Tick()
	{
		int j;
		if (self.shade != -32 && self.shade != -33)
		{
			if (self.scale.X != 0)
				j = self.ifhitbyweapon();
			else
				j = -1;

			if (j >= 0 || self.shade == -31)
			{
				if (j >= 0) self.lotag = 0;

				self.temp_data[3] = 1;

				DukeStatIterator it;
				for(let act2 = it.first(STAT_STANDABLE); act2; act2 = it.Next())
				{
					if (self.hitag == act2.hitag && act2.actorflag2(SFLAG2_BRIGHTEXPLODE))
						act2.shade = -32;
				}
			}
		}
		else
		{
			if (self.shade == -32)
			{
				if (self.lotag > 0)
				{
					self.lotag -= 3;
					if (self.lotag <= 0) self.lotag = -99;
				}
				else
				{
					self.shade = -33;
				}
			}
			else
			{
				if (self.scale.X > 0)
				{
					self.temp_data[2]++;
					if (self.temp_data[2] == 3)
					{
						if (self.spritesetindex < self.getSpriteSetSize() - 1)
						{
							self.temp_data[2] = 0;
							self.setSpriteSetImage(self.spritesetindex + 1);
						}
						else
						{
							self.detonate('DukeExplosion2');
						}
					}
					return;
				}
				self.detonate('DukeExplosion2');
			}
		}
	}

}

class DukeSeenine : DukeOozFilter
{
	default
	{
		spriteset "SEENINE", "SEENINEDEAD", "SEENINEDEAD1";
		-GREENBLOOD;
	}

}


