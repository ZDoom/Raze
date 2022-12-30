
class RedneckBikerBV2 : DukeActor
{
	default
	{
		pic "BIKERBV2";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
	}
	override void Initialize()
	{
		self.scale = (0.4375, 0.34375);
		self.clipdist = 18;
	}
}

class RedneckBikerB : DukeActor
{
	default
	{
		pic "BIKERB";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
	}
	override void Initialize()
	{
		self.scale = (0.4375, 0.34375);
		self.clipdist = 18;
	}
}

class RedneckBiker : DukeActor
{
	default
	{
		pic "BIKER";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
	}
	override void Initialize()
	{
		self.scale = (0.4375, 0.34375);
		self.setClipDistFromTile();
	}
}

class RedneckMakeout : DukeActor
{
	default
	{
		pic "MAKEOUT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
	}
	override void Initialize()
	{
		self.scale = (0.40625, 0.40625);
		self.setClipDistFromTile();
	}
}

class RedneckCheerleaderB : DukeActor
{
	default
	{
		pic "CHEERB";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
	}
	override void Initialize()
	{
		self.scale = (0.4375, 0.34375);
		self.clipdist = 18;
	}
}

class RedneckCheerleader : DukeActor
{
	default
	{
		pic "CHEER";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		+ALTPROJECTILESPRITE; // owed to CON's shittiness. Todo: Think of something better.
		jumptoplayer_factor 1.6;
}
	override void Initialize()
	{
		self.scale = (0.3125, 0.3125);
		self.setClipDistFromTile();
	}
}

class RedneckCootplay : DukeActor
{
	default
	{
		pic "COOTPLAY";
		+INTERNAL_BADGUY;
		+LOOKALLAROUND;
		+NORADIUSPUSH;
	}
	override void Initialize()
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
	}
	override void Initialize()
	{
		self.scale = (0.390625, 0.328125);
		self.setClipDistFromTile();
	}
}

class RedneckMinionBoat : DukeActor
{
	default
	{
		pic "MINIONBOAT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		+DONTDIVE;
		falladjustz 3;
	}
	override void Initialize()
	{
		self.scale = (0.25, 0.25);
		self.setClipDistFromTile();
	}
}

class RedneckHulkBoat : DukeActor
{
	default
	{
		pic "HULKBOAT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		+DONTDIVE;
		falladjustz 12;
	}
	override void Initialize()
	{
		self.scale = (0.75, 0.75);
		self.setClipDistFromTile();
	}
}

class RedneckCheerBoat : DukeActor
{
	default
	{
		pic "CHEERBOAT";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		+DONTDIVE;
		falladjustz 6;
	}
	override void Initialize()
	{
		self.scale = (0.5, 0.5);
		self.setClipDistFromTile();
	}
}

class RedneckRock : DukeActor
{
	default
	{
		pic "ROCK";
		+INTERNAL_BADGUY;
	}
	override void Initialize()
	{
		self.scale = (1, 1);
		self.setClipDistFromTile();
	}
}

class RedneckRock2 : RedneckRock
{
	default
	{
		pic "ROCK2";
		+INTERNAL_BADGUY;
	}
}

class RedneckMamaCloud : DukeActor
{
	default
	{
		pic "MAMACLOUD";
		+NORADIUSPUSH;
	}
	override void Initialize()
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
	}

	override void Initialize()
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

class RedneckSBSwipe : DukeActor
{
	default
	{
		pic "SBSWIPE";
		+BADGUYSTAYPUT;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
	}
	
	override void initialize()
	{
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
		self.scale = (0.390625, 0.328125);
		self.setClipDistFromTile();
	}
}

class RedneckCheerStayput : RedneckCheerleader
{
	default
	{
		pic "CHEERSTAYPUT";
		jumptoplayer_factor 2.0;
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
}

class RedneckShitBoss : DukeActor
{
	default
	{
		pic "SBMOVE";
		+FULLBRIGHT;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NORADIUSPUSH;
	}

	override void Initialize()
	{
		self.scale = (0.75, 0.75);
		self.setClipDistFromTile();
	}
}

class RedneckBubbaStand : DukeActor
{
	default
	{
		pic "BUBBASTAND";
		+INTERNAL_BADGUY;
	}
	
	override void initialize()
	{
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
		self.scale = (0.390625, 0.328125);
		self.setClipDistFromTile();
	}
}


class RedneckHulk : DukeActor
{
	default
	{
		pic "HULK";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NORADIUSPUSH;
		ProjectileSpread -5.625;
	}

	override void Initialize()
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
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
}

class RedneckBoulder : DukeActor
{
	default
	{
		pic "BOULDER";
		+INTERNAL_BADGUY;
	}
}

class RedneckBoulder1 : DukeActor
{
	default
	{
		pic "BOULDER1";
		+INTERNAL_BADGUY;
	}
}

class RedneckTornado : DukeActor
{
	default
	{
		pic "TORNADO";
		+INTERNAL_BADGUY;
		+NOHITSCANHIT;
	}
	override void Initialize()
	{
		self.scale = (1, 2);
		self.setClipDistFromTile();
		self.clipdist *= 0.25;
		self.cstat = CSTAT_SPRITE_TRANSLUCENT;
	}
}

class RedneckDog : DukeActor
{
	default
	{
		pic "DOGRUN";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
	}
	override void Initialize()
	{
		self.scale = (0.25, 0.25);
		self.setClipDistFromTile();
	}
}

class RedneckSheriff : DukeActor
{
	default
	{
		pic "LTH";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
	}
	override void Initialize()
	{
		self.scale =  (0.375, 0.34375);
		self.setClipDistFromTile();
	}
}

class RedneckMosquito : DukeActor
{
	default
	{
		pic "DRONE";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NOWATERDIP;
		+FLOATING;
		+QUICKALTERANG;
		+NOJIBS;
		falladjustz 0;
		floating_floordist 30;
		floating_ceilingdist 50;
	}
	override void Initialize()
	{
		self.scale = (0.21875, 0.109375);
		self.clipdist = 32;
	}
}

// only new thing in Route 66.
class RedneckGator : DukeActor
{
	default
	{
		pic "GATOR";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
	}
}