class RedneckMakeout : DukeActor
{
	default
	{
		pic "MAKEOUT";
		+INTERNAL_BADGUY;
		//+KILLCOUNT;
		+LOOKALLAROUND;
		Strength 150;
	}
	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.40625, 0.40625);
		self.setClipDistFromTile();
	}

}