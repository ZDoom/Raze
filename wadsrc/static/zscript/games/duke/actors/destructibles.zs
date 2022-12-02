
class DukeGenericPole2 : DukeActor
{
	default
	{
		pic "GENERICPOLE2";
	}
	
	override void OnHit(DukeActor proj)
	{
		for (int k = 0; k < 6; k++)
		{
			let a = frandom(0, 360);
			let vel = frandom(4, 8);
			let zvel = -frandom(0, 16) - self.vel.Z * 0.25;
			let spawned = dlevel.SpawnActor(self.sector, self.pos.plusZ(-8), 'DukeScrap', -8, (0.75, 0.75), a, vel, zvel, self, STAT_MISC);
			if (spawned) spawned.spriteextra = DukeScrap.Scrap1 + random(0, 15);
		}
		self.PlayActorSound("GLASS_HEAVYBREAK");
		self.Destroy();
	}
}

class DukeHangLight : DukeGenericPole2
{
	default
	{
		statnum STAT_ACTOR;
		clipdist 8;
		pic "HANGLIGHT";
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}

}

class DukeStatueFlash : DukeActor
{
	default
	{
		pic "STATUEFLASH";
	}
	
	override void OnHit(DukeActor proj)
	{
		self.lotsofcolourglass(40);
		self.PlayActorSound("GLASS_HEAVYBREAK");
		self.angle = frandom(0, 360);
		self.lotsofglass(8);
		self.Destroy();
	}
}

class DukeStatue : DukeStatueFlash
{
	default
	{
		pic "STATUE";
	}
}

	
class DukeBottle10 : DukeActor
{
	Default
	{
		pic "BOTTLE10";
		clipdist 8;
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}
	override void OnHit(DukeActor proj)
	{
		self.lotsofstuff("DukeMoney", random(4, 7));
		self.PlayActorSound("GLASS_HEAVYBREAK");
		self.angle = frandom(0, 360);
		self.lotsofglass(8);
		self.Destroy();
	}
}


class DukeVase : DukeActor
{
	Default
	{
		pic "VASE";
		clipdist 8;
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}
	override void OnHit(DukeActor proj)
	{
		self.lotsofglass(49);
		self.PlayActorSound("GLASS_HEAVYBREAK");
		self.angle = frandom(0, 360);
		self.lotsofglass(8);
		self.Destroy();
	}

}

class DukeFanSprite : DukeActor
{
	Default
	{
		spriteset "FANSPRITE", "FANSPRITEBROKE";
		clipdist 8;
		statnum STAT_DEFAULT;
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}
	override void OnHit(DukeActor proj)
	{
		if (self.spritesetindex == 0)
		{
			self.setSpriteSetImage(1);
			self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
			if (self.sector.CheckTexture(sectortype.floor, "FANSHADOW"))
				self.sector.SetTexture(sectortype.floor, "FANSHADOWBROKE");

			self.PlayActorSound("GLASS_HEAVYBREAK");
			for (int j = 0; j < 16; j++) self.RANDOMSCRAP();
		}
	}
}

class DukeSatellite : DukeActor
{
	Default
	{
		pic "SATELITE";
		clipdist 8;
		statnum STAT_DEFAULT;
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}
	override void OnHit(DukeActor proj)
	{
		if (!proj.actorflag3(SFLAG3_LIGHTDAMAGE))
		{
			for (int j = 0; j < 15; j++)
			{
				let a = frandom(0, 360);
				let vel = frandom(4, 12);
				let zvel = -frandom(-1, 1);

				let spawned = dlevel.SpawnActor(self.sector, (self.pos.XY, self.sector.floorz - 12 - j * 2), 'DukeScrap', -8, (1, 1), a, vel, zvel, self, STAT_MISC);
				if (spawned) spawned.spriteextra = DukeScrap.Scrap1 + random(0, 15);

			}
			self.spawn("DukeExplosion2");
			self.Destroy();
		}
	}
}

class DukeFuelPod : DukeSatellite
{
	Default
	{
		pic "FUELPOD";
	}
}

class DukeSolarPanel : DukeSatellite
{
	Default
	{
		pic "SOLARPANNEL";
	}
}

class DukeAntenna : DukeSatellite
{
	Default
	{
		pic "ANTENNA";
	}
}

class DukeFetus : DukeActor
{
	Default
	{
		spriteset "FETUS", "FETUSBROKE";
		clipdist 8;
		statnum STAT_DEFAULT;
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}
	override void OnHit(DukeActor proj)
	{
		Console.printf("a%d", self.spritesetindex);
		
		if (self.spritesetindex == 0)
		{
			Console.printf("a%d", proj.spawnindex);
			self.setSpriteSetImage(1);
			self.PlayActorSound("GLASS_BREAKING");
			self.lotsofglass(10);
		}
		else
		{
			Console.printf("b%d", proj.spawnindex);
			for (int j = 0; j < 48; j++)
			{
				self.shoot("DukeBloodSplat1");
				self.angle += 58.5; // Was 333, which really makes no sense.
			}
			self.PlayActorSound("GLASS_HEAVYBREAK");
			self.PlayActorSound("SQUISHED");
			self.PlayActorSound("GLASS_BREAKING");
			self.lotsofglass(10);
			self.Destroy();
		}
	}
}

class DukeFetusBroke : DukeFetus
{
	Default
	{
		spritesetindex 1;
	}
}

// This one had no init code.
class DukeHydroplant : DukeActor
{
	Default
	{
		spriteset "HYDROPLANT", "BROKEHYDROPLANT";
	}
	
	override void OnHit(DukeActor proj)
	{
		if (self.spritesetindex == 0)
		{
			self.setSpriteSetImage(1);
			self.PlayActorSound("GLASS_BREAKING");
			self.lotsofglass(10);
		}
		else if (self.cstat & CSTAT_SPRITE_BLOCK)
		{
			self.PlayActorSound("GLASS_BREAKING");
			self.pos.Z += 16;
			self.cstat = 0;
			self.lotsofglass(5);
		}
	}
}

class DukeHydroplantBroke : DukeHydroplant
{
	Default
	{
		spritesetindex 1;
	}
}
