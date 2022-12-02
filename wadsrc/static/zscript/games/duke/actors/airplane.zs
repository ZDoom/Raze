class RedneckAirplane : DukeActor
{
	default
	{
		statnum STAT_ACTOR;
		pic "AIRPLANE";
		ScaleX 1;
		ScaleY 1;
	}
	
	override void Initialize()
	{
		self.extra = self.lotag;
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
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
