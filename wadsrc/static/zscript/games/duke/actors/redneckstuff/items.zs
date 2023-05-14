extend class DukeItemBase
{
	const CASULAMMOAMOUNT        = 6;
	const SHOTGUNAMMOAMOUNT_RR      = 10;
	const RIFLEAMMOAMOUNT        = 30;
	const CROSSBOWAMMOBOX        = 5;
	const CHICKENBOWAMMOBOX      = 5;
	const ALIENBLASTERAMMOAMOUNT = 33;
	const DYNAMITEBOX            = 5;
	
	const MOONSHINE_AMOUNT        = 400;
	const SHIELD_AMOUNT           = 100;
	const SCUBA_AMOUNT            = 6400;
	const HEAT_AMOUNT             = 1200;
	const COWPIE_AMOUNT           = 600;
	const BEER_AMOUNT             = 2400;
	const WHISKEY_AMOUNT          = MAXPLAYERHEALTH;
	const BOOT_AMOUNT             = 2000;

	const POWDERKEGBLASTRADIUS    = 3880;
	
}

class RedneckMoonshine : DukeItemBase
{
	default
	{
		pic "STEROIDS";
		+INVENTORY;
	}
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.203125, 0.140625));
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
class RedneckCowpie : DukeItemBase
{
	default
	{
		pic "COWPIE";
		+INVENTORY;
	}
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.125, 0.09375));
	}
}
class RedneckBeer : DukeItemBase
{
	default
	{
		pic "BEER";
	}
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.078125, 0.0625));
	}
}
class RedneckSnorkel : DukeItemBase
{
	default
	{
		pic "AIRTANK";
		+INVENTORY;
	}

	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.296875, 0.25));
	}
}
class RedneckDoorkey : DukeAccessCard
{
	override void Initialize(DukeActor spawner)
	{
		super.Initialize(spawner);
		self.Scale = (0.171875, 0.1875);
	}
}
class RedneckAmmo : DukeItemBase
{
	default
	{
		pic "AMMO";
	}
	
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.140625, 0.140625));
	}

}
class RedneckTitAmmo : DukeItemBase
{
	default
	{
		pic "TEATAMMO";
	}
}
class RedneckSawAmmo : DukeItemBase
{
	default
	{
		pic "SAWAMMO";
	}
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.1875, 0.109375));
	}
}
class RedneckShotgunammo : DukeItemBase
{
	default
	{
		pic "SHOTGUNAMMO";
	}
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.28125, 0.265625));
		if (isRRRA()) self.cstat = CSTAT_SPRITE_BLOCK_HITSCAN;
	}
}

class RedneckRifleAmmo : DukeItemBase
{
	default
	{
		pic "BATTERYAMMO";
	}
	
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.234375, 0.234375));
	}
}
class RedneckBlaster : DukeItemBase
{
	default
	{
		pic "ALIENBLASTERSPRITE";
	}

	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.28125, 0.265625));
	}
}
class RedneckBlasterammo : DukeItemBase
{
	default
	{
		pic "DEVISTATORAMMO";
	}

	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.15625, 0.140625));
	}
}

class RedneckBowlingBallsprite : DukeItemBase
{
	default
	{
		pic "BOWLINGBALLSPRITE";
		strength BOWLINGBALL_WEAPON_STRENGTH;
	}
	
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.171875, 0.171875), -1, true);
	}
}
class RedneckDynamiteAmmo : DukeItemBase
{
	default
	{
		pic "HBOMBAMMO";
	}
}
class RedneckCrossbow : DukeItemBase
{
	default
	{
		pic "CROSSBOWSPRITE";
	}
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.25, 0.21875));
	}
}
class RedneckShotgun : DukeItemBase
{
	default
	{
		pic "SHOTGUNSPRITE";
	}
}
class RedneckRipsaw : DukeItemBase
{
	default
	{
		pic "RIPSAWSPRITE";
	}

	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.34375, 0.203125));
	}
}
class RedneckTitgun : DukeItemBase
{
	default
	{
		pic "TITSPRITE";
	}

	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.265625, 0.25));
	}
}
class RedneckPorkRinds : DukeItemBase
{
	default
	{
		pic "SIXPAK";
	}
	
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.203125, 0.140625));
		if (isRRRA()) self.cstat = CSTAT_SPRITE_BLOCK_HITSCAN;
	}
}
class RedneckGoogooCluster : DukeItemBase
{
	default
	{
		pic "ATOMICHEALTH";
		+FULLBRIGHT;
		+BIGHEALTH;
		+NOFLOORPAL;
	}
	
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.125, 0.125));
		self.cstat |= CSTAT_SPRITE_YCENTER;
	}
	
	override bool animate(tspritetype t)
	{
		t.pos.Z -= 4;
		return false;
	}
}
class RedneckWhiskey : DukeItemBase
{
	default
	{
		pic "FIRSTAID";
		+INVENTORY;
	}
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.125, 0.125));
	}
}
class RedneckRevolver : DukeItemBase
{
	default
	{
		pic "FIRSTGUNSPRITE";
	}
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, ((0.25, 0.25)));
	}
}
class RedneckPowderKeg : DukeItemBase
{
	default
	{
		pic "POWDERKEG";
		+NOFLOORPAL;
		+EXPLOSIVE;
		+DOUBLEDMGTHRUST;
		+BREAKMIRRORS;
		+INFLAME;
	}
	
	override void Tick()
	{
		let sectp = self.sector;
		if (sectp.lotag != ST_1_ABOVE_WATER && (!(ud.mapflags & MFLAG_ALLSECTORTYPES) && sectp.lotag != ST_160_FLOOR_TELEPORT))
			if (self.vel.X != 0)
			{
				movesprite((self.Angle.ToVector()* self.vel.X, self.vel.Z), CLIPMASK0);
				self.vel.X -= 1. / 16.;
			}
		Super.Tick();
	}
	
	
	override bool shootthis(DukeActor actor, DukePlayer p, Vector3 spos, double sang) const
	{
		let j = actor.spawn("RedneckPowderKeg");
		if (j)
		{
			j.vel.X = 2;
			j.Angle = actor.Angle;
			j.pos.Z -= 5;
		}
		return true;
	}
}
class RedneckRiflegun : DukeItemBase
{
	default
	{
		pic "RIFLEGUNSPRITE";
	}
}

class RedneckMotoAmmo : DukeItemBase
{
	default
	{
		pic "MOTOAMMO";
	}
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.359375, 0.359375));
	}
}
class RedneckBoatAmmo : DukeItemBase
{
	default
	{
		pic "BOATAMMO";
	}
	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.359375, 0.359375));
	}
}
class RedneckChickenArrows : DukeItemBase
{
	default
	{
		pic "RPG2SPRITE";
	}

	override void Initialize(DukeActor spawner)
	{
		commonItemSetup(spawner, (0.34375, 0.3125));
	}
}
