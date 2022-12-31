class RedneckVixen : DukeActor
{
	default
	{
		pic "VIXEN";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		ProjectileSpread -2.8125;
	}

	override void Initialize()
	{
		if (self.pal == 34)
		{
			self.scale = (0.34375, 0.328125);
		}
		else
		{
			self.scale = (0.3125, 0.3125);
		}
		self.setClipDistFromTile();
	}

	
	override Vector3 SpecialProjectileOffset()
	{
		return (0, 0, -12);
	}
	
}


