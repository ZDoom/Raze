
// Generic class that can handle most simple destructible items without writing any new code at all. All actions are defined in .def files in the spawnclasses block.
class DukeGenericDestructible : DukeActor
{
	TextureID spawnstate, brokenstate;
	Sound breaksound;
	int fullbright;
	int broken;
	
	enum EFlags
	{
		f_damaging = 1,
		f_solid = 2,
		f_unblocking = 4,
		f_spawnglass = 8,
		f_spawnscrap = 16,
		f_spawnsmoke = 32,
		f_spawnglass2 = 64
	}
	
	native bool SetBroken(bool bust);	// sets broken texture. Must be done natively as long as we do not have proper texture support.
	
	virtual bool DestroyAction() { return false; }	// for customized subclasses
	
	override void Initialize()
	{
		if (self.inittype & f_solid) self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.SetBroken(false);
	}
	
	override void OnHit(DukeActor proj)
	{
		if (broken) return;
		
		broken = true;
		if (breaksound >= 0) self.PlayActorSound(breaksound);
		bool res1 = self.SetBroken(true);
		bool res2 = DestroyAction();
		let flags = self.inittype;
		if (flags & f_unblocking) self.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		if (flags & f_damaging) self.hitradius(10, 0, 0, 1, 1);
		if (flags & f_spawnglass) self.lotsofglass(10);
		if (flags & f_spawnscrap) for(int i = 0; i < 16; i++) self.RandomScrap();
		if (flags & f_spawnsmoke) self.spawn('DukeSmallSmoke');
		if (flags & f_spawnglass2)
		{
			self.angle = frandom(0, 360);
			self.lotsofglass(8);
		}

		if (res1 || res2) 
			self.Destroy();
	}
	
	override bool Animate(tspritetype tspr)
	{
		if (fullbright & (1 << broken)) tspr.shade = -127;
		return true;
	}
}

