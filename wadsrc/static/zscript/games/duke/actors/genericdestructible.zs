

class DukeGenericDestructible : DukeActor
{
	TextureID spawnstate, brokenstate;
	Sound breaksound;
	int fullbright;
	int broken;
	
	native bool SetBroken(bool bust);	// sets broken texture. Must be done natively as long as we do not have proper texture support.
	
	virtual bool DestroyAction() { return false; }	// for customized subclasses
	
	override void Initialize()
	{
		self.SetBroken(false);
	}
	
	override void OnHit(DukeActor proj)
	{
		if (broken) return;
		
		broken = true;
		bool res1 = self.SetBroken(true);
		bool res2 = DestroyAction();
		if (res1 || res2) 
			self.Destroy();
	}
	
	override bool Animate(tspritetype tspr)
	{
		if (fullbright & (1 << broken)) tspr.shade = -127;
		return true;
	}
}

class DukeGenericDamagingDestructible : DukeGenericDestructible
{
	override bool DestroyAction() 
	{
		self.hitradius(10, 0, 0, 1, 1);
		return false;
	}
}

class DukeGenericGlassSpawningDestructible : DukeGenericDestructible
{
	override bool DestroyAction() 
	{
		self.lotsofglass(10);
		return false;
	}
}

class DukeGenericScrapSpawningDestructible : DukeGenericDestructible
{
	override bool DestroyAction() 
	{
		for(int i = 0; i < 16; i++) self.RandomScrap();
		self.Destroy();
		return false;
	}
}

class DukeGenericUnblockingDestructible : DukeGenericDestructible
{
	override bool DestroyAction() 
	{
		self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		return false;
	}
}

class DukeGenericSolidScrapSpawningDestructible : DukeGenericDestructible
{
	override void Initialize()
	{
		self.SetBroken(false);
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}
	
	override bool DestroyAction() 
	{
		for(int i = 0; i < 16; i++) self.RandomScrap();
		self.Destroy();
		return false;
	}
}

class DukeGenericSolidUnblockingDestructible : DukeGenericDestructible
{
	override void Initialize()
	{
		self.SetBroken(false);
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
	}
	
	override bool DestroyAction() 
	{
		self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		return false;
	}
}
