class RedneckBikerB : DukeActor
{
	default
	{
		pic "BIKERB";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		watermovefactor 0.5;
		gravityfactor 0.25;
		Strength 300;
	}
	override void Initialize()
	{
		self.scale = (0.4375, 0.34375);
		self.clipdist = 18;
	}

}