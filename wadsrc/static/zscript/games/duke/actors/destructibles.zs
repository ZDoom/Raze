
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
