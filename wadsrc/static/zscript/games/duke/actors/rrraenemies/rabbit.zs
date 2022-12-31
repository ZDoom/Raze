class RedneckRabbit : DukeActor
{
	default
	{
		pic "RABBIT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		Strength 50;
	}
	
	override void Initialize()
	{
		self.scale = (0.28125, 0.28125);
	}
}
