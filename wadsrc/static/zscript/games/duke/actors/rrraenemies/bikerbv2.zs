
class RedneckBikerBV2 : DukeActor
{
	default
	{
		pic "BIKERBV2";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		watermovefactor 0.5;
		gravityfactor 0.125;
		Strength 200;
	}
	override void Initialize()
	{
		self.scale = (0.4375, 0.34375);
		self.clipdist = 18;
	}

}
