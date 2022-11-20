class RedneckRabbitSpawner : DukeActor
{
	default
	{
		statnum STAT_FX;
		extra 0;
	}
	
	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
	}
	
	override void Tick()
	{
		if (self.statnum == STAT_RABBITSPAWN)
		{
			if (self.hitag > 0)
			{
				if (self.extra == 0)
				{
					self.hitag--;
					self.extra = 150;
					self.spawn("RedneckRabbit");
				}
				else
					self.extra--;
			}
		}
	}
	
	override void onRespawn(int low)
	{
		if (self.lotag == low)
		{
			if (!ud.monsters_off)
				self.ChangeStat(STAT_RABBITSPAWN);
		}
	}
}



