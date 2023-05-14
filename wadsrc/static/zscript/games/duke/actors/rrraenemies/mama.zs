class RedneckMamaCloud : DukeActor
{
	default
	{
		pic "MAMACLOUD";
		Strength 10000;
		+NORADIUSPUSH;
	}
	override void Initialize(DukeActor spawner)
	{
		self.scale = (1, 1);
		self.cstat = CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_TRANS_FLIP;
		self.pos.X += frandom(-64, 64);
		self.pos.Y += frandom(-64, 64);
		self.pos.Z += frandom(-4, 4);
	}
	
}

class RedneckMama : DukeActor
{
	default
	{
		pic "MAMA";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		+NORADIUSPUSH;
		+SPAWNRABBITGUTS; // owed to CON's shittiness. Todo: Think of something better.
		justjump1_factor 1.83;
		justjump2_factor 2.286;
		Strength 2000;
	}

	override void Initialize(DukeActor spawner)
	{
		if (self.pal == 30)
		{
			self.scale = (0.40625, 0.40625);
			self.clipdist = 18.75;
		}
		else if (self.pal == 31)
		{
			self.scale = (0.5625, 0.5625);
			self.clipdist = 25;
		}
		else if (self.pal == 32)
		{
			self.scale = (0.78125, 0.78125);
			self.clipdist = 25;
		}
		else
		{
			self.scale = (0.78125, 0.78125);
			self.clipdist = 25;
		}
	}
	
}

