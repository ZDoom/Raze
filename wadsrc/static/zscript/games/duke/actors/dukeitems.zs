class DukeItemBase : DukeActor
{
	override void Initialize()
	{
		commonItemSetup();
	}	
}


class DukeCrystalAmmo : DukeItemBase
{
	default
	{
		pic "CRYSTALAMMO";
		+NOFLOORPAL;
	}
	
	override bool animate(tspritetype t)
	{
		t.shade = int(Raze.BobVal(PlayClock << 4) * 16);
		return false;
	}
}

class DukeFlamethrowerSprite : DukeItemBase
{
	default
	{
		pic "FLAMETHROWERSPRITE";
	}
}

class DukeFlamethrowerAmmo : DukeItemBase
{
	default
	{
		pic "FLAMETHROWERAMMO";
	}
}

class DukeSteroids : DukeItemBase
{
	default
	{
		pic "STEROIDS";
		+INVENTORY;
	}
}

class DukeHeatSensor : DukeItemBase
{
	default
	{
		pic "HEATSENSOR";
		+INVENTORY;
	}
}

class DukeShield : DukeItemBase
{
	default
	{
		pic "SHIELD";
	}
}

class DukeAirtank : DukeItemBase
{
	default
	{
		pic "AIRTANK";
		+INVENTORY;
	}
}

class DukeTripbombSprite : DukeItemBase
{
	default
	{
		pic "TRIPBOMBSPRITE";
	}
}

class DukeJetpack : DukeItemBase
{
	default
	{
		pic "JETPACK";
		+INVENTORY;
	}
}

class DukeHoloDuke : DukeItemBase
{
	default
	{
		pic "HOLODUKE";
		+INVENTORY;
	}
}

class DukeFirstgunSprite : DukeItemBase
{
	default
	{
		pic "FIRSTGUNSPRITE";
	}
}

class DukeShotgunSprite : DukeItemBase
{
	default
	{
		pic "SHOTGUNSPRITE";
	}
}
class DukeChaingunSprite : DukeItemBase
{
	default
	{
		pic "CHAINGUNSPRITE";
	}
}
class DukeRPGSprite : DukeItemBase
{
	default
	{
		pic "RPGSPRITE";
	}
}
class DukeShrinkerSprite : DukeItemBase
{
	default
	{
		pic "SHRINKERSPRITE";
	}
}
class DukeFreezeSprite : DukeItemBase
{
	default
	{
		pic "FREEZESPRITE";
	}
}
class DukeDevastatorSprite : DukeItemBase
{
	default
	{
		pic "DEVISTATORSPRITE";
	}
}

class DukeShotgunammo : DukeItemBase
{
	default
	{
		pic "SHOTGUNAMMO";
	}
}

class DukeFreezeammo : DukeItemBase
{
	default
	{
		pic "FREEZEAMMO";
	}
}
class DukeHBombammo : DukeItemBase
{
	default
	{
		pic "HBOMBAMMO";
	}
}
class DukeGrowammo : DukeItemBase
{
	default
	{
		pic "GROWAMMO";
	}
}
class DukeDevastatorammo : DukeItemBase
{
	default
	{
		pic "DEVISTATORAMMO";
	}
}
class DukeRPGammo : DukeItemBase
{
	default
	{
		pic "RPGAMMO";
	}
}
class DukeAmmo : DukeItemBase
{
	default
	{
		pic "AMMO";
	}
	
	override void Initialize()
	{
		commonItemSetup((0.25, 0.25));
	}
	
}

class DukeBoots : DukeItemBase
{
	default
	{
		pic "BOOTS";
		+INVENTORY;
	}
}

class DukeAmmoLots : DukeItemBase
{
	default
	{
		pic "AMMOLOTS";
	}
}

class DukeCola : DukeItemBase
{
	default
	{
		pic "COLA";
	}
}

class DukeFirstAid : DukeItemBase
{
	default
	{
		pic "FIRSTAID";
		+INVENTORY;
	}
}

class DukeAccessCard : DukeItemBase
{
	default
	{
		pic "ACCESSCARD";
	}
	
	override void Initialize()
	{
		if (ud.multimode > 1 && ud.coop != 1)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
			return;
		}

		self.Scale = (0.5, 0.5);
		self.shade = -17;

		if (!self.mapSpawned) self.ChangeStat(STAT_ACTOR);
		else
		{
			self.ChangeStat(STAT_ZOMBIEACTOR);
			self.makeitfall();
		}
	}
}


