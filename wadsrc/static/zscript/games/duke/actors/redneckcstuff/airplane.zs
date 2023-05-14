class RedneckAirplane : DukeActor
{
	default
	{
		pic "AIRPLANE";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.extra = self.lotag;
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.Scale = (1, 1);
		self.ChangeStat(STAT_ACTOR);
	}

	override void Tick()
	{
		if (self.extra)
		{
			if (self.extra == self.lotag)
				Duke.PlaySound("PLANE");
			self.extra--;
			int j = self.movesprite((self.angle.ToVector() * self.hitag / 16., self.hitag / 128.), CLIPMASK0);
			if (j > 0)
			{
				self.PlayActorSound("PIPEBOMB_EXPLODE");
				self.Destroy();
			}
			if (self.extra == 0)
			{
				Duke.PlaySound("PLANEXP");
				self.Destroy();
				ud.earthquaketime = 32;
				Duke.GetViewPlayer().pals = Color(32, 32, 32, 48);
			}
		}
	}
}
