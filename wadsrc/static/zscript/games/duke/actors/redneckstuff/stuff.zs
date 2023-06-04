class RedneckRock : DukeActor
{
	default
	{
		pic "ROCK";
		+INTERNAL_BADGUY;
		Strength 200;
	}
	override void Initialize(DukeActor spawner)
	{
		self.scale = (1, 1);
		self.setClipDistFromTile();
	}
	
}

class RedneckRock2 : DukeActor
{
	default
	{
		pic "ROCK2";
		+INTERNAL_BADGUY;
		Strength 200;
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

class RedneckTeslaBall : DukeItemBase
{
	default
	{
		Strength MORTER_WEAPON_STRENGTH;
		pic "TESLABALL";
	}
}

class RedneckTesla : DukeItemBase
{
	default
	{
		pic "TESLA";
		+FULLBRIGHT
	}
	
}


class RedneckUfoBeam : DukeActor
{
	default
	{
		pic "UFOBEAM";
		+BADGUY;
		ProjectileSpread 0;
		Strength 100;
	}
	
	override bool animate(tspritetype t)
	{
		t.cstat |= CSTAT_SPRITE_INVISIBLE;
		self.cstat |= CSTAT_SPRITE_INVISIBLE;
		return true;
	}
}

class RedneckTikiLamp : DukeActor
{
	default
	{
		pic "TIKILAMP";
		+FULLBRIGHT
		Strength REALLYTOUGH;
	}
}

class DukeMinecartKiller : DukeActor
{
	default
	{
		+BADGUY;
		Strength MEGASTRENGTH;
	}
	override void StaticSetup()
	{
		self.cstat |= CSTAT_SPRITE_INVISIBLE;
	}
	
}


class RedneckBustaWin4a : DukeItemBase
{
	default
	{
		pic "BUSTAWIN4A";
		Strength WEAK;
	}
}

class RedneckBustaWin5a : DukeItemBase
{
	default
	{
		pic "BUSTAWIN5A";
		Strength WEAK;
	}

}

class RedneckBarrel : DukeActor
{
	const POWDERKEGBLASTRADIUS = 3880;
	const POWDERKEG_STRENGTH = 100;

	default
	{
		spriteset "BARREL", "BARRELBROKE";
	}
}
class RedneckBarrel2 : RedneckBarrel
{
	default
	{
		spriteset "BARREL2", "BARREL2BROKE";
	}
}


class RedneckWaterSurface : DukeActor
{
	default
	{
		pic "WATERSURFACE";
	}
}
