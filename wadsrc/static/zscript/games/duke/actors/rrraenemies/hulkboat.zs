class RedneckHulkBoat : DukeActor
{
	default
	{
		pic "HULKBOAT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		+DONTDIVE;
		falladjustz 12;
		landmovefactor 0.5;
		Strength 300;
	}
	override void Initialize()
	{
		self.scale = (0.75, 0.75);
		self.setClipDistFromTile();
	}

}
