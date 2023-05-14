extend class DukeActor
{
}

class RedneckHulk : DukeActor
{
	const HULKSTRENGTH = 1200;
	const HULKRESPAWNEDSTRENGTH = 600;
	const HULKWHACKAMOUNT = -22;
	
	default
	{
		pic "HULK";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NORADIUSPUSH;
		ProjectileSpread -5.625;
		precacheclass "RedneckHulkJibA", "RedneckHulkJibB", "RedneckHulkJibC";
		Strength HULKSTRENGTH;
	}

	override void Initialize(DukeActor spawner)
	{
		self.scale = (0.5, 0.5);
		self.setClipDistFromTile();
	}

	override Vector3 SpecialProjectileOffset()
	{
		return ((self.Angle + 45).ToVector() * 16, 12);
	}

}

class RedneckHulkStayput : RedneckHulk
{
	default
	{
		pic "HULKSTAYPUT";
		+BADGUYSTAYPUT;
	}
	
	
}
