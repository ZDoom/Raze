class RedneckHen : DukeActor
{
	default
	{
		pic "HEN";
		+INTERNAL_BADGUY;
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

class RedneckHenStayput: RedneckHen
{
	default
	{
		pic "HENSTAYPUT";
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
}
