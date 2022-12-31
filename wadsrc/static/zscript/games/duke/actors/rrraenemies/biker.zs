
class RedneckBiker : DukeActor
{
	default
	{
		pic "BIKER";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		Strength 150;
	}
	override void Initialize()
	{
		self.scale = (0.4375, 0.34375);
		self.setClipDistFromTile();
	}

}
