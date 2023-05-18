class RedneckTornado : DukeActor
{
	default
	{
		pic "TORNADO";
		+DESTRUCTOIMMUNE;
		+INTERNAL_BADGUY;
		+NOHITSCANHIT;
		+NOSHADOW;
		Strength MEGASTRENGTH;
	}
	override void Initialize(DukeActor spawner)
	{
		self.scale = (1, 2);
		self.setClipDistFromTile();
		self.clipdist *= 0.25;
		self.cstat = CSTAT_SPRITE_TRANSLUCENT;
	}
}