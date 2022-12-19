class DukeRespawnController : DukeActor
{
	override void Initialize()
	{
		if (ud.multimode < 2 && self.pal == 1)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
		}
		else
		{
			self.cstat = CSTAT_SPRITE_INVISIBLE;
			self.extra = 66 - 13;
			self.ChangeStat(STAT_FX);
		}
	}

	override void Tick()
	{
		if (self.extra == 66)
		{
			let newact = self.spawnsprite(self.hitag);
			if (Raze.isRRRA() && newact)
			{
				newact.pal = self.pal;

				// RRRA's ghost monsters.
				if (newact.pal == 8)
				{
					newact.cstat |= CSTAT_SPRITE_TRANSLUCENT;
				}

				if (newact.pal != 6)
				{
					self.Destroy();
					return;
				}
				self.extra = (66 - 13);
				newact.pal = 0;
			}
			else
			{
				self.Destroy();
			}
		}
		else if (self.extra > (66 - 13))
			self.extra++;
	}
	
	override void onRespawn(int low)
	{
		if (self.lotag == low)
		{
			if (Duke.badguyID(self.hitag) && ud.monsters_off) return;

			let star = self.spawn("DukeTransporterStar");
			if (star)
			{
				star.pos.Z -= 32;
			}
			self.extra = 66 - 12;   // Trigger the spawn countdown
		}
	}
}

