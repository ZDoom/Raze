
class DukeStatueFlash : DukeActor
{
	default
	{
		pic "STATUEFLASH";
		+HITRADIUSCHECK;
	}

	override void Initialize()
	{
		self.clipdist = 32;
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
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
		+TRIGGERRESPAWN;
	}
}

