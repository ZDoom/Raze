class RedneckCheerleaderB : DukeActor
{
	default
	{
		pic "CHEERB";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		watermovefactor 0.5;
		gravityfactor 0.25;
		Strength 150;
	}
	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.4375, 0.34375);
		self.clipdist = 18;
	}
}