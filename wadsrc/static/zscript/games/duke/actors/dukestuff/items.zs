class DukeItemBase : DukeActor
{
	const PISTOLAMMOAMOUNT        = 12;
	const SHOTGUNAMMOAMOUNT       = 10;
	const CHAINGUNAMMOAMOUNT      = 50;
	const RPGAMMOBOX              = 5;
	const CRYSTALAMMOAMOUNT       = 5;
	const GROWCRYSTALAMMOAMOUNT   = 20;
	const DEVISTATORAMMOAMOUNT    = 15;
	const FREEZEAMMOAMOUNT        = 25;
	const FLAMETHROWERAMMOAMOUNT  = 25;
	const HANDBOMBBOX             = 5;
	const PIG_SHIELD_AMOUNT1      = 75;
	const PIG_SHIELD_AMOUNT2      = 50;
	const MAXPLAYERATOMICHEALTH   = 200;



	override void Initialize()
	{
		commonItemSetup();
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

class DukeBoots : DukeItemBase
{
	default
	{
		pic "BOOTS";
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

class DukeHoloDuke : DukeItemBase
{
	default
	{
		pic "HOLODUKE";
		+INVENTORY;
		Strength 0;
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

		self.scale = (0.5, 0.5);
		self.shade = -17;
		if (!self.mapSpawned) self.ChangeStat(STAT_ACTOR);
		else
		{
			self.ChangeStat(STAT_ZOMBIEACTOR);
			self.makeitfall();
		}
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

class DukeFreezeammo : DukeItemBase
{
	default
	{
		pic "FREEZEAMMO";
	}
}

class DukeShotgunammo : DukeItemBase
{
	default
	{
		pic "SHOTGUNAMMO";
	}

}

class DukeAmmoLots : DukeItemBase
{
	default
	{
		pic "AMMOLOTS";
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

class DukeGrowammo : DukeItemBase
{
	default
	{
		pic "GROWAMMO";
	}
}

class DukeBatteryAmmo : DukeItemBase
{
	default
	{
		pic "BATTERYAMMO";
	}
	
	override void Initialize()
	{
		commonItemSetup();
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

class DukeHBombammo : DukeItemBase
{
	default
	{
		pic "HBOMBAMMO";
	}
}

class DukeRPGSprite : DukeItemBase
{
	default
	{
		pic "RPGSPRITE";
	}
}

class DukeShotgunSprite : DukeItemBase
{
	default
	{
		pic "SHOTGUNSPRITE";
	}
}

class DukeSixpak : DukeItemBase
{
	default
	{
		pic "SIXPAK";
	}
}

class DukeCola : DukeItemBase
{
	default
	{
		pic "COLA";
	}
}

class DukeAtomicHealth : DukeItemBase
{
	default
	{
		pic "ATOMICHEALTH";
		+FULLBRIGHT;
		+BIGHEALTH;
		+NOFLOORPAL;
	}
	
	override void Initialize()
	{
		commonItemSetup();
		self.cstat |= CSTAT_SPRITE_YCENTER;
	}
	
	override bool animate(tspritetype t)
	{
		t.pos.Z -= 4;
		return false;
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

class DukeFirstgunSprite : DukeItemBase
{
	default
	{
		pic "FIRSTGUNSPRITE";
	}
}

class DukeTripbombSprite : DukeItemBase
{
	default
	{
		pic "TRIPBOMBSPRITE";
	}
}

class DukeChaingunSprite : DukeItemBase
{
	default
	{
		pic "CHAINGUNSPRITE";
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
