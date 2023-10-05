class BloodMissileBase : BloodActor
{
	meta double speed;
	meta double defclipdist;
	meta double angleofs;
	meta int callbackID;	// make this a function name later
	meta int spawnsoundID;
	meta Sound spawnsound;
	meta double movementAdd;
	meta double randomVel;
	meta int seqID;
	meta name seqName;
	meta int seqCallbackID;	// make this a function name later
	
	property prefix: none;
	property speed: speed;
	property clipdist: defclipdist;
	property angleofs: angleofs;
	property callbackID: callbackID;
	property spawnsound: spawnsound;
	property spawnsoundID: spawnsoundID;
	property movementAdd: movementAdd;
	property randomVel: randomVel;
	property seqID: seqID;
	property seqName: seqName;
	property seqCallbackID: seqCallbackID;
	
	default
	{
		callbackID -1;
		seqID -1;
		spawnsoundID -1;
		seqCallbackID -1;
	}
	
	virtual void initMissile(BloodActor spawner)
	{
		self.vel += self.movementAdd * spawner.vel;
		if (self.randomVel > 0)
		{
			self.vel += (Blood.Random2F(self.randomVel), Blood.Random2F(self.randomVel), Blood.Random2F(self.randomVel) );
		}
		if (self.seqName != 'None')
		{
			// todo: the game cannot handle named seqs yet.
		}
		else if (self.seqID > -1)
		{
			self.seqSpawnID(self.seqID, self.seqCallbackID);
		}
		if (self.callbackID > -1)
		{
			self.evPostActorCallback(0, self.callbackID);
		}
		if (self.spawnsound != 0)
		{
			// todo: currently only IDs supported.
			//self.play3DSound(self.spawnsound, 0, 0);
		}
		else if (self.spawnsoundID > -1)
		{
			self.play3DSoundID(self.spawnsoundID, 0, 0);
		}
	}
	
}

class BloodMissileButcherKnife : BloodMissileBase
{
	default
	{
		pic "ButcherKnife";
		speed 14.933319;
		angleOfs 90.000000;
		scale 0.625000, 0.625000;
		shade -16;
		clipdist 4.000000;
	}
	
	override void initMissile(BloodActor spawner)
	{
		super.initMissile(spawner);
		self.cstat |= CSTAT_SPRITE_ALIGNMENT_WALL;
	}
	
}
class BloodMissileFlareRegular : BloodMissileBase
{
	default
	{
		pic "FLAREBURST";
		speed 48.000000;
		scale 0.500000, 0.500000;
		shade -128;
		clipdist 8.000000;
		callbackID kCallbackFXFlareSpark;
		spawnsoundID 422;
	}
}
class BloodMissileTeslaAlt : BloodMissileBase
{
	default
	{
		pic "Fireball";
		speed 42.666656;
		scale 0.500000, 0.500000;
		shade -128;
		clipdist 8.000000;
		callbackID kCallbackFXTeslaAlt;
	}
}
class BloodMissileFlareAlt : BloodMissileBase
{
	default
	{
		pic "FLAREBURST";
		speed 37.333328;
		scale 0.500000, 0.500000;
		shade -128;
		clipdist 1.000000;
		callbackID kCallbackFXFlareSpark;
		spawnsoundID 422;
	}
		
	override void initMissile(BloodActor spawner)
	{
		super.initMissile(spawner);
		self.evPostActorCallback(30, kCallbackFXFlareBurst);
	}
}
class BloodMissileFlameSpray : BloodMissileBase
{
	default
	{
		speed 17.066666;
		scale 0.375000, 0.375000;
		shade -128;
		clipdist 4.000000;
		movementAdd 0.5;
		RandomVel 0x11111;
	}
	
	override void initMissile(BloodActor spawner)
	{
		super.InitMissile(spawner);
		int index = Blood.Chance(0x8000)? 1 : 0;
		self.seqSpawnID(index, -1);
	}
	
}
class BloodMissileFireball : BloodMissileBase
{
	default
	{
		speed 17.066666;
		scale 0.500000, 0.500000;
		shade -128;
		clipdist 8.000000;
		spawnsoundID 441;
		seqID 22;
		seqCallbackID nFireballClient;
	}
}
class BloodMissileTeslaRegular : BloodMissileBase
{
	default
	{
		pic "Teslaball";
		speed 42.666656;
		scale 0.500000, 0.500000;
		shade -128;
		clipdist 4.000000;
		spawnsoundID 251;
	}
}
class BloodMissileEctoSkull : BloodMissileBase
{
	default
	{
		pic "EctoSkull";
		speed 10.666656;
		scale 0.500000, 0.500000;
		shade -24;
		clipdist 8.000000;
		spawnsoundID 493;
	}
}
class BloodMissileFlameHound : BloodMissileBase
{
	default
	{
		speed 17.066666;
		scale 0.375000, 0.375000;
		shade -128;
		clipdist 4.000000;
		seqID 27;
		movementAdd 0.5;
		randomVel 0x11111;
	}
}
class BloodMissilePukeGreen : BloodMissileBase
{
	default
	{
		speed 12.799988;
		scale 0.250000, 0.250000;
		shade -16;
		clipdist 4.000000;
		seqID 29;
	}
}
class BloodMissileUnused : BloodMissileBase
{
	default
	{
		speed 12.799988;
		scale 0.125000, 0.125000;
		clipdist 4.000000;
	}
}
class BloodMissileArcGargoyle : BloodMissileBase
{
	default
	{
		pic "Fireball";
		speed 32.000000;
		scale 0.500000, 0.500000;
		shade -128;
		clipdist 4.000000;
		spawnsoundID 252;
	}
}
class BloodMissileFireballNapalm : BloodMissileBase
{
	default
	{
		speed 37.333328;
		scale 0.468750, 0.468750;
		shade -128;
		clipdist 6.000000;
		spawnsoundID 441;
		seqID 61;
		seqCallbackID nNapalmClient;
	}
}
class BloodMissileFireballCerberus : BloodMissileBase
{
	default
	{
		speed 37.333328;
		scale 0.468750, 0.468750;
		shade -128;
		clipdist 6.000000;
		seqID 61;
		seqCallbackID dword_2192E0;
	}
}
class BloodMissileFireballTchernobog : BloodMissileBase
{
	default
	{
		speed 21.333328;
		scale 0.375000, 0.375000;
		shade -128;
		clipdist 4.000000;
		seqID 23;
		seqCallbackID dword_2192D8;
		movementAdd 0.5;
		RandomVel 0x11111;
	}
}
class BloodMissileLifeLeechRegular : BloodMissileBase
{
	default
	{
		pic "LeechBall";
		speed 42.666656;
		scale 0.500000, 0.500000;
		shade -128;
		clipdist 4.000000;
		callbackID kCallbackFXFlameLick;
	}
}
class BloodMissileLifeLeechAltNormal : BloodMissileBase
{
	default
	{
		pic "Fireball";
		speed 37.333328;
		scale 0.250000, 0.250000;
		shade -128;
		clipdist 4.000000;
	}
}
class BloodMissileLifeLeechAltSmall : BloodMissileBase
{
	default
	{
		pic "Fireball";
		speed 26.666656;
		scale 0.500000, 0.500000;
		shade -128;
		clipdist 4.000000;
		callbackID kCallbackFXArcSpark;
	}
}


