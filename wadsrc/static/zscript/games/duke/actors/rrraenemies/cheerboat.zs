class RedneckCheerBoat : DukeActor
{
	default
	{
		pic "CHEERBOAT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		+DONTDIVE;
		falladjustz 6;
		landmovefactor 0.5;
		Strength 200;
	}
	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.5, 0.5);
		self.setClipDistFromTile();
	}
}
