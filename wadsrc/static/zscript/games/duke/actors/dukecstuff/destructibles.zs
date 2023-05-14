
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
		pic "HANGLIGHT";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.clipdist = 8;
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.ChangeStat(STAT_ACTOR);
	}

}

	
class DukeBottle10 : DukeActor
{
	Default
	{
		pic "BOTTLE10";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
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
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
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
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
		self.ChangeStat(STAT_DEFAULT);
	}
	override void OnHit(DukeActor proj)
	{
		if (self.spritesetindex == 0)
		{
			self.setSpriteSetImage(1);
			self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
			if (self.sector.CheckTexture(sectortype.floor, "FANSHADOW"))
				self.sector.SetTextureName(sectortype.floor, "FANSHADOWBROKE");

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
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
		self.ChangeStat(STAT_DEFAULT);
	}
	override void OnHit(DukeActor proj)
	{
		if (!proj.bLIGHTDAMAGE)
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
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
		self.ChangeStat(STAT_DEFAULT);
	}
	override void OnHit(DukeActor proj)
	{
		if (self.spritesetindex == 0)
		{
			self.setSpriteSetImage(1);
			self.PlayActorSound("GLASS_BREAKING");
			self.lotsofglass(10);
		}
		else
		{
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

class DukeHydrant : DukeActor
{
	Default
	{
		spriteset "HYDRENT", "BROKEFIREHYDRENT";
		precacheclass "DukeToiletWater";
		+INFLAME;
		+DOUBLEDMGTHRUST;
		+BREAKMIRRORS;
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
		self.ChangeStat(STAT_DEFAULT);
	}
	override void OnHit(DukeActor proj)
	{
		if (self.spritesetindex == 0)
		{
			self.setSpriteSetImage(1);
			self.spawn("DukeToiletWater");
			self.PlayActorSound("GLASS_HEAVYBREAK");
		}
	}
}

class DukePipe1 : DukeActor
{
	Default
	{
		spriteset "PIPE1", "PIPE1B";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
		self.ChangeStat(STAT_DEFAULT);
	}
	override void OnHit(DukeActor proj)
	{
		if (self.spritesetindex == 0)
		{
			self.setSpriteSetImage(1);
			let spawned = self.spawn("DukeSteam");
			if (spawned) spawned.pos.Z = self.sector.floorz - 32;
		}
	}
}

class DukePipe2 : DukePipe1
{
	Default
	{
		spriteset "PIPE2", "PIPE2B";
	}
}

class DukePipe3 : DukePipe1
{
	Default
	{
		spriteset "PIPE3", "PIPE3B";
	}
}

class DukePipe4 : DukePipe1
{
	Default
	{
		spriteset "PIPE4", "PIPE4B";
	}
}

class DukePipe5 : DukePipe1
{
	Default
	{
		spriteset "PIPE5", "PIPE5B";
	}
}

class DukePipe6 : DukePipe1
{
	Default
	{
		spriteset "PIPE6", "PIPE6B";
	}
}


class DukeSpaceMarine : DukeActor
{
	Default
	{
		pic "SPACEMARINE";
		+HITRADIUSCHECK;
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.extra = 20;
		ChangeStat(STAT_ZOMBIEACTOR);
	}
	override void OnHit(DukeActor proj)
	{
		self.extra -= proj.extra;
		if (self.extra > 0) return;
		self.angle = frandom(0, 360);
		self.shoot("DukeBloodSplat1");
		self.angle = frandom(0, 360);
		self.shoot("DukeBloodSplat2");
		self.angle = frandom(0, 360);
		self.shoot("DukeBloodSplat3");
		self.angle = frandom(0, 360);
		self.shoot("DukeBloodSplat4");
		self.angle = frandom(0, 360);
		self.shoot("DukeBloodSplat1");
		self.angle = frandom(0, 360);
		self.shoot("DukeBloodSplat2");
		self.angle = frandom(0, 360);
		self.shoot("DukeBloodSplat3");
		self.angle = frandom(0, 360);
		self.shoot("DukeBloodSplat4");
		self.spawnguts("DukeJibs1", 1);
		self.spawnguts("DukeJibs2", 2);
		self.spawnguts("DukeJibs3", 3);
		self.spawnguts("DukeJibs4", 4);
		self.spawnguts("DukeJibs5", 1);
		self.spawnguts("DukeJibs3", 6);
		Duke.PlaySound("SQUISHED");
		self.Destroy();
	}
}

class DukeMonk : DukeSpaceMarine
{
	default
	{
		pic "MONK";
		-HITRADIUSCHECK;
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
		ChangeStat(STAT_DEFAULT);
	}
	
	override void OnHit(DukeActor proj)
	{
		self.PlayActorSound(Raze.FindSoundByResID(self.lotag));
		self.spawnsprite(self.hitag);
		super.OnHit(proj);
	}
}

class DukeLuke : DukeMonk
{
	default
	{
		pic "LUKE";
	}
}

class DukeIndy : DukeMonk
{
	default
	{
		pic "INDY";
	}
}

class DukeJuryGuy : DukeMonk
{
	default
	{
		pic "JURYGUY";
	}
}

class DukeChair3 : DukeActor
{
	default
	{
		pic "CHAIR3";
		+ALWAYSROTATE1;
	}
	
	override void OnHit(DukeActor proj)
	{
		self.PlayActorSound("GLASS_HEAVYBREAK");
		for (int j = 0; j < 16; j++) self.RANDOMSCRAP();
		self.Destroy();
	}
}


class RedneckFlamingo : DukeActor
{
	default
	{
		spriteset "FLAMINGO", "FLAMINGOB";
	}
	
	override void OnHit(DukeActor proj)
	{
		if (self.spritesetindex < self.getSpriteSetSize() - 1)
		{
			self.setSpriteSetImage(self.spritesetindex + 1);
			self.PlayActorSound("GLASS_BREAKING");
			self.lotsofglass(10);
			for (int k = 0; k < 6; k++)
			{
				let a = frandom(0, 360);
				let vel = frandom(4, 8);
				let zvel = -frandom(0, 16) - self.vel.Z * 0.25;

				let spawned = dlevel.SpawnActor(self.sector, self.pos.plusZ(-8), "DukeScrap", -8, (0.75, 0.75), a, vel, zvel, self, STAT_MISC);
				if (spawned) spawned.spriteextra = DukeScrap.Scrap6 + random(0, 15);
			}
		}
	}
}

class RedneckMarbleStatue : RedneckFlamingo
{
	default
	{
		spriteset "MARBLESTATUE1", "MARBLESTATUE2", "MARBLESTATUE3";
	}
}

class RedneckMarbleStatue2 : RedneckMarbleStatue
{
	default
	{
		spritesetindex 1;
	}
}

class RedneckSnakeRiverSign : DukeActor
{
	default
	{
		spriteset "FLAMINGO", "FLAMINGOB";
	}
	
	override void OnHit(DukeActor proj)
	{
		if (self.spritesetindex == 0)
		{
			self.setSpriteSetImage(1);
				
			self.PlayActorSound("WOODBREK");
			self.hitradius(10, 0, 0, 1, 1);
			if (self.lotag != 0)
			{
				DukeSpriteIterator it;
				for (let act = it.First(); act; act = it.Next())
				{
					if (act is 'RedneckSnakeRiverSign' && act.pal == 4)
					{
						if (act.lotag == self.lotag)
							act.setSpriteSetImage(1);
					}
				}
			}
		}
	}
}
