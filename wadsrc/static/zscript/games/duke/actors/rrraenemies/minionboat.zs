class RedneckMinionBoat : DukeActor
{
	default
	{
		pic "MINIONBOAT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		+DONTDIVE;
		falladjustz 3;
		landmovefactor 0.5;
		Strength 150;
	}
	override void Initialize()
	{
		self.scale = (0.25, 0.25);
		self.setClipDistFromTile();
	}

}
