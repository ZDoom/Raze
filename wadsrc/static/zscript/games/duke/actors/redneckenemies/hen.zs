class RedneckHen : DukeActor
{
	const HEN_NORMAL_STRENGTH = 5;
	const HEN_TOUGHER_STRENGTH = 12;
	const HEN_DAMAGE_TO_PLAYER = -1;

	default
	{
		pic "HEN";
		+INTERNAL_BADGUY;
		Strength HEN_TOUGHER_STRENGTH;
	}
	
	override void Initialize()
	{
		if (self.pal == 35)
		{
			self.scale = (0.65625, 0.46875);
			self.setClipDistFromTile();
		}
		else
		{
			self.scale = (0.328125, 0.234375);
			self.clipdist = 16;
		}
	}
	

}
