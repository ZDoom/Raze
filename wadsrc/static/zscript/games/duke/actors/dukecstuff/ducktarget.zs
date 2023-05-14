class DukeLetter : DukeActor
{
	default
	{
		pic "LETTER";
		+NOFALLER;
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.ChangeStat(STAT_ACTOR);
		self.extra = 1;
	}
}

class DukeDuck : DukeLetter // shooting gallery target
{
	default
	{
		pic "DUCK";
	}
	override void Tick()
	{
		if (self.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)
		{
			self.counter++;
			if (self.counter > 60)
			{
				self.counter = 0;
				self.cstat = CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_BLOCK_ALL | CSTAT_SPRITE_ALIGNMENT_WALL;
				self.extra = 1;
			}
		}
		else
		{
			int j = self.ifhitbyweapon();
			if (j >= 0)
			{
				self.cstat = CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_YCENTER;

				DukeStatIterator itr;
				for(let act2 = itr.First(STAT_ACTOR); act2; act2 = itr.Next())
				{
					if (act2.lotag == self.lotag && act2.GetClass() == self.GetClass())
					{
						if ((act2.hitag && !(act2.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)) ||
							(!act2.hitag && (act2.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR))
							)
							return;
					}
				}

				// got the last one. Receive your reward!
				dlevel.operateactivators(self.lotag, nullptr);
				self.operateforcefields(self.lotag);
				dlevel.operatemasterswitches(self.lotag);
			}
		}
	}
}

class DukeTarget : DukeDuck
{
	default
	{
		pic "TARGET";
	}
}
