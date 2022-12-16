
class RedneckChickenArrows : DukeItemBase
{
	default
	{
		pic "RPG2SPRITE";
	}

	override void Initialize()
	{
		commonItemSetup((0.34375, 0.3125));
	}
}
	
class RedneckMotoAmmo : DukeItemBase
{
	default
	{
		pic "MOTOAMMO";
	}
	override void Initialize()
	{
		commonItemSetup((0.359375, 0.359375));
	}
}
	
class RedneckBoatAmmo : DukeItemBase
{
	default
	{
		pic "BOATAMMO";
	}
	override void Initialize()
	{
		commonItemSetup((0.359375, 0.359375));
	}
}

class RedneckMoonshine : DukeItemBase
{
	default
	{
		pic "STEROIDS";
		+INVENTORY;
	}
	override void Initialize()
	{
		commonItemSetup((0.203125, 0.140625));
	}
}
	
class RedneckSnorkel : DukeItemBase
{
	default
	{
		pic "AIRTANK";
		+INVENTORY;
	}

	override void Initialize()
	{
		commonItemSetup((0.296875, 0.25));
	}
}

class RedneckCowpie : DukeItemBase
{
	default
	{
		pic "COWPIE";
		+INVENTORY;
	}
	override void Initialize()
	{
		commonItemSetup((0.125, 0.09375));
	}
}
	
class RedneckSixpack : DukeItemBase
{
	default
	{
		pic "HOLODUKE";
		+INVENTORY;
	}
}

class RedneckRevolver : DukeItemBase
{
	default
	{
		pic "FIRSTGUNSPRITE";
	}
	override void Initialize()
	{
		commonItemSetup(((0.25, 0.25)));
	}
}
	
class RedneckRiflegun : DukeItemBase
{
	default
	{
		pic "RIFLEGUNSPRITE";
	}
}

class RedneckShotgun : DukeItemBase
{
	default
	{
		pic "SHOTGUNSPRITE";
	}
}

class RedneckCrossbow : DukeItemBase
{
	default
	{
		pic "CROSSBOWSPRITE";
	}
	override void Initialize()
	{
		commonItemSetup((0.25, 0.21875));
	}

}
	
class RedneckRipsaw : DukeItemBase
{
	default
	{
		pic "RIPSAWSPRITE";
	}

	override void Initialize()
	{
		commonItemSetup((0.34375, 0.203125));
	}
}
	
class RedneckTitgun : DukeItemBase
{
	default
	{
		pic "TITSPRITE";
	}

	override void Initialize()
	{
		commonItemSetup((0.265625, 0.25));
	}
}
	
class RedneckBlaster : DukeItemBase
{
	default
	{
		pic "ALIENBLASTERSPRITE";
	}

	override void Initialize()
	{
		commonItemSetup((0.28125, 0.265625));
	}
}

class RedneckShotgunammo : DukeItemBase
{
	default
	{
		pic "SHOTGUNAMMO";
	}
	override void Initialize()
	{
		commonItemSetup((0.28125, 0.265625));
		if (Raze.isRRRA()) self.cstat = CSTAT_SPRITE_BLOCK_HITSCAN;
	}
}
	
class RedneckDynamiteAmmo : DukeItemBase
{
	default
	{
		pic "HBOMBAMMO";
	}
}

class RedneckBlasterammo : DukeItemBase
{
	default
	{
		pic "DEVISTATORAMMO";
	}

	override void Initialize()
	{
		commonItemSetup((0.15625, 0.140625));
	}
}
	
class RedneckHipWader : DukeItemBase
{
	default
	{
		pic "BOOTS";
		+INVENTORY;
	}
}

class RedneckAmmo : DukeItemBase
{
	default
	{
		pic "AMMO";
	}
	
	override void Initialize()
	{
		commonItemSetup((0.140625, 0.140625));
	}
}	
	
class RedneckBeer : DukeItemBase
{
	default
	{
		pic "BEER";
	}
	override void Initialize()
	{
		commonItemSetup((0.078125, 0.0625));
	}
}
	
class RedneckWhiskey : DukeItemBase
{
	default
	{
		pic "FIRSTAID";
		+INVENTORY;
	}
	override void Initialize()
	{
		commonItemSetup((0.125, 0.125));
	}
}

class RedneckSawAmmo : DukeItemBase
{
	default
	{
		pic "SAWAMMO";
	}
	override void Initialize()
	{
		commonItemSetup((0.1875, 0.109375));
	}
}

class RedneckTitAmmo : DukeItemBase
{
	default
	{
		pic "TEATAMMO";
	}
}


class RedneckDoorkey : DukeAccessCard
{
	override void Initialize()
	{
		super.Initialize();
		self.Scale = (0.171875, 0.1875);
	}
}

		