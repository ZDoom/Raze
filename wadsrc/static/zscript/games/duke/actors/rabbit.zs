class RedneckRabbit : DukeActor
{
	default
	{
		pic "RABBIT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
	}
	
	override void Initialize()
	{
		self.scale = (0.28125, 0.28125);
	}
}
