class RedneckCootplay : DukeActor
{
	default
	{
		pic "COOTPLAY";
		+INTERNAL_BADGUY;
		+LOOKALLAROUND;
		+NORADIUSPUSH;
		Strength 10000;
	}
	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.375, 0.28128);
		self.setClipDistFromTile();
		self.clipdist *= 4;
	}
	
}

class RedneckBillyPlay : DukeActor
{
	default
	{
		pic "BILLYPLAY";
		+INTERNAL_BADGUY;
		+LOOKALLAROUND;
		+NORADIUSPUSH;
		Strength 10000;
	}
	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.390625, 0.328125);
		self.setClipDistFromTile();
	}
		
}
