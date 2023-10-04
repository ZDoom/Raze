class BloodItemBase : BloodActor
{
	meta int packslot;
	meta int respawntype;
	Property prefix: none;
	property packslot: packslot;
	property respawntype: respawntype;
	
	default
	{
		respawntype 0;
	}


	override int getRespawnTime()
	{
		if (!self.hasX) return -1;
		if (self.xspr.respawn == 3 && gGameOptions.nGameType == Blood.kSinglePlayer) return 0;
		else if (self.xspr.respawn == 2 || (self.xspr.respawn != 1 && gGameOptions.nItemSettings != Blood.ITEMSETTINGS_0))
		{
			switch (self.respawntype)
			{
			case 1:
				return gGameOptions.nSpecialRespawnTime;
			case 2:
				return gGameOptions.nSpecialRespawnTime << 1;
			default:
				return gGameOptions.nItemRespawnTime;
			}
		}
		return -1;
	}
}

class BloodKeyBase : BloodItemBase
{
}

// items (keys)
class BloodItemKeySkull : BloodKeyBase
{
	default
	{
		pic "HudKeyIcon1";
		shade -8;
		scale 0.5, 0.5;
	}
}

class BloodItemKeyEye : BloodKeyBase
{
	default
	{
		pic "HudKeyIcon2";
		shade -8;
		scale 0.5, 0.5;
	}
	
}

class BloodItemKeyFire : BloodKeyBase
{
	default
	{
		pic "HudKeyIcon3";
		shade -8;
		scale 0.5, 0.5;
	}
	
}

class BloodItemKeyDagger : BloodKeyBase
{
	default
	{
		pic "HudKeyIcon4";
		shade -8;
		scale 0.5, 0.5;
	}
	
}

class BloodItemKeySpider : BloodKeyBase
{
	default
	{
		pic "HudKeyIcon5";
		shade -8;
		scale 0.5, 0.5;
	}
	
}

class BloodItemKeyMoon : BloodKeyBase
{
	default
	{
		pic "HudKeyIcon6";
		shade -8;
		scale 0.5, 0.5;
	}
	
}

class BloodItemKeyKey7 : BloodKeyBase
{
	default
	{
		pic "HudKeyIcon7";
		shade -8;
		scale 0.5, 0.5;
	}
	
}


// items (health)
class BloodItemHealthDoctorBag : BloodItemBase
{
	default
	{
		pic "Pack2Icon1";
		shade -8;
		scale 0.75, 0.75;
		packslot 0;
	}
}

class BloodItemHealthMedPouch : BloodItemBase
{
	default
	{
		pic "MedPouchIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemHealthLifeEssense : BloodItemBase
{
	default
	{
		pic "Healthicon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemHealthLifeSeed : BloodItemBase
{
	default
	{
		pic "LifeSeedIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemHealthRedPotion : BloodItemBase
{
	default
	{
		pic "RedPotionIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}


// items (misc)
class BloodItemFeatherFall : BloodItemBase
{
	default
	{
		pic "FeatherFallIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemShadowCloak : BloodItemBase
{
	default
	{
		pic "ShadowCloakIcon";
		shade -8;
		scale 0.625, 0.625;
		respawntype 1;
	}
}

class BloodItemDeathMask : BloodItemBase
{
	default
	{
		pic "DeathMaskIcon";
		shade -8;
		scale 0.625, 0.625;
		respawntype 2;
	}
}

class BloodItemJumpBoots : BloodItemBase
{
	default
	{
		pic "Pack2Icon5";
		shade -8;
		scale 0.625, 0.625;
		packslot 4;
	}
}

class BloodItemTwoGuns : BloodItemBase
{
	default
	{
		pic "GunsAkimboIcon";
		shade -8;
		scale 0.625, 0.625;
		respawntype 1;
	}
}

class BloodItemDivingSuit : BloodItemBase
{
	default
	{
		pic "Pack2Icon2";
		shade -8;
		scale 1.25, 1.25;
		packslot 1;
	}
}

class BloodItemGasMask : BloodItemBase
{
	default
	{
		pic "GasMaskIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemCrystalBall : BloodItemBase
{
	default
	{
		pic "Pack2Icon3";
		shade -8;
		scale 0.625, 0.625;
		packslot 2;
	}
}

class BloodItemReflectShots : BloodItemBase
{
	default
	{
		pic "ReflectiveIcon";
		shade -8;
		scale 0.625, 0.625;
		respawntype 1;
	}
}

class BloodItemBeastVision : BloodItemBase
{
	default
	{
		pic "Pack2Icon4";
		shade -8;
		scale 0.625, 0.625;
		packslot 3;
	}
}

class BloodItemShroomDelirium : BloodItemBase
{
	default
	{
		pic "DeliriumIcon";
		shade -8;
		scale 0.75, 0.75;
	}
}


class BloodItemArmorAsbest : BloodItemBase
{
	default
	{
		pic "AsbestIcon";
		shade -8;
		scale 1.25, 1;
	}
}

class BloodItemArmorBasic : BloodItemBase
{
	default
	{
		pic "BasicArmorIcon";
		shade -8;
		scale 1, 1;
	}
}

class BloodItemArmorBody : BloodItemBase
{
	default
	{
		pic "Armor3Icon";
		shade -8;
		scale 1, 1;
	}
}

class BloodItemArmorFire : BloodItemBase
{
	default
	{
		pic "Armor1Icon";
		shade -8;
		scale 1, 1;
	}
}

class BloodItemArmorSpirit : BloodItemBase
{
	default
	{
		pic "Armor2Icon";
		shade -8;
		scale 1, 1;
	}
}

class BloodItemArmorSuper : BloodItemBase
{
	default
	{
		pic "SuperArmorIcon";
		shade -8;
		scale 1, 1;
	}
}


class BloodItemFlagABase : BloodItemBase
{
	default
	{
		pic "FlagBaseIcon";
		shade -8;
		scale 1, 1;
	}
}

class BloodItemFlagBBase : BloodItemBase
{
	default
	{
		pic "FlagBaseIcon";
		shade -8;
		pal 7;
		scale 1, 1;
	}
}

class BloodFlagBase : BloodActor
{
}

class BloodItemFlagA : BloodFlagBase
{
	default
	{
		pic "FlagIcon";
		shade -128;
		scale 1, 1;
	}
}

class BloodItemFlagB : BloodFlagBase
{
	default
	{
		pic "FlagIcon";
		shade -128;
		pal 7;
		scale 1, 1;
	}
}


// others
class BloodItemRavenFlight : BloodItemBase
{
	default
	{
		pic "RavenFlightIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemClone : BloodItemBase
{
	default
	{
		pic "CloneIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemDecoy : BloodItemBase
{
	default
	{
		pic "DecoyIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemDoppleganger : BloodItemBase
{
	default
	{
		pic "DoppleIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemShadowCloakUseless : BloodItemBase
{
	default
	{
		pic "UselessIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemRageShroom : BloodItemBase
{
	default
	{
		pic "RageShroomIcon";
		shade -8;
		scale 0.75, 0.75;
	}
}

class BloodItemGrowShroom : BloodItemBase
{
	default
	{
		pic "GrowShroomIcon";
		shade -8;
		scale 0.75, 0.75;
	}
}

class BloodItemShrinkShroom : BloodItemBase
{
	default
	{
		pic "ShrinkShroomIcon";
		shade -8;
		scale 0.75, 0.75;
	}
}

class BloodItemDeathMaskUseless : BloodItemBase
{
	default
	{
		pic "UselessIcon2";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemWineGoblet : BloodItemBase
{
	default
	{
		pic "WineGobletIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemWineBottle : BloodItemBase
{
	default
	{
		pic "WineBottleIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemSkullGrail : BloodItemBase
{
	default
	{
		pic "SkullGrailIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemSilverGrail : BloodItemBase
{
	default
	{
		pic "SilverGrailIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemTome : BloodItemBase
{
	default
	{
		pic "TomeIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemBlackChest : BloodItemBase
{
	default
	{
		pic "BlackChestIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

class BloodItemWoodenChest : BloodItemBase
{
	default
	{
		pic "WoodenChestIcon";
		shade -8;
		scale 0.625, 0.625;
	}
}

