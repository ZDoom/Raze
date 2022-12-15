
class RedneckBikerBV2 : DukeActor
{
	default
	{
		pic "BIKERBV2";
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
	}
	override void Initialize()
	{
		self.scale = (0.34375, 0.3125);
		self.setClipDistFromTile();
	}
}

class RedneckCootplay : DukeActor
{
	default
	{
		pic "COOTPLAY";
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
	}
}

class RedneckMamaCloud : DukeActor
{
	default
	{
		pic "MAMACLOUD";
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
	}

	override void Initialize()
	{
		self.scale = (0.75, 0.75);
		self.setClipDistFromTile();
	}

	override bool animate(tspritetype t)
	{
		t.shade = -127;
		return false;
	}
}

class RedneckBubbaStand : DukeActor
{
	default
	{
		pic "BUBBASTAND";
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
	}

	override void Initialize()
	{
		self.scale = (0.5, 0.5);
		self.setClipDistFromTile();
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
	}
}

class RedneckBoulder1 : DukeActor
{
	default
	{
		pic "BOULDER1";
	}
}

class RedneckTornado : DukeActor
{
	default
	{
		pic "TORNADO";
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
	}
	override void Initialize()
	{
		self.scale = (0.21875, 0.109375);
		self.clipdist = 32;
	}
}

class RedneckVixen : DukeActor
{
	default
	{
		pic "VIXEN";
	}
}
