class RedneckVixen : DukeActor
{
	const VIXENWACKAMOUNT = -10;
	const VIXEN_STRENGTH = 800;
	const VIXENSNDAMB = 1;
	const GREEN = 22;
	const BROWN = 15;
	const BLUE = 0;
	const QUEEN = 34;
	const SPECIAL = 25;

	default
	{
		pic "VIXEN";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		ProjectileSpread -2.8125;
		Strength VIXEN_STRENGTH;
		
		
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


