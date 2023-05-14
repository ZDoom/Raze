class RedneckShitBoss : DukeActor
{
	const SBSPITSPD = 16;
	const SBDIPSPD = 16;
	const SBSNDRND = 64;
	const SBSWHACKAMOUNT = -22;

	default
	{
		pic "SBMOVE";
		+FULLBRIGHT;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NORADIUSPUSH;
		Strength 2500;
	}

	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.75, 0.75);
		self.setClipDistFromTile();
	}

}