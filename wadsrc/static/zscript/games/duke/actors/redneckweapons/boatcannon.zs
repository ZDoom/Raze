//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class RedneckBoatGrenade : RedneckDynamiteArrow // RRRA only
{
	default
	{
		pic "BOATGRENADE";
		-DOUBLEDMGTHRUST;
		-ALWAYSROTATE2;
		-DOUBLEHITDAMAGE;
		DukeProjectile.SpawnSound "MORTAR";
	}
	
	override void Initialize()
	{
		
		self.extra = 10;
		self.vel.Z = -10;
		self.vel.X *= 2;
		super.Initialize();
	}

	override bool premoveeffect()
	{
		if (self.extra)
		{
			self.vel.Z = -(self.extra * 250/256.); // 250 looks like a typo...
			self.extra--;
		}
		else
			self.makeitfall();
		
		return Super.premoveeffect();
	}
	
	override void posthiteffect(CollisionData coll)
	{
		self.rpgexplode(coll.type, oldpos, false, 160, "RPG_EXPLODE");
		self.Destroy();
	}
	
	override class<DukeActor> GetRadiusDamageType(int targhealth)
	{
		return 'DukeRadiusExplosion';
	}
	
}

