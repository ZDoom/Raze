class BloodMissileBase : BloodActor
{
	meta double speed;
	meta double angleofs;
	meta VMFunction callback;
	meta Sound spawnsound;
	meta double movementAdd;
	meta double randomVel;
	meta int seqID;
	meta name seqName;
	meta VMFunction seqCallback;
	
	property prefix: none;
	property speed: speed;
	property angleofs: angleofs;
	property callback: callback;
	property spawnsound: spawnsound;
	property movementAdd: movementAdd;
	property randomVel: randomVel;
	property seqID: seqID;
	property seqName: seqName;
	property seqCallback: seqCallback;
	
	default
	{
		seqID -1;
	}
	
	virtual void initMissile(BloodActor spawner)
	{
		self.cstat2 |= CSTAT2_SPRITE_MAPPED;
		self.shade = self.defshade;
		self.pal = self.defpal;
		self.clipdist = clipdist;
		self.flags = kPhysMove;

		self.Angle = spawner.angle + self.angleofs;
		self.ownerActor = spawner;
		self.cstat |= CSTAT_SPRITE_BLOCK;
		self.xspr.target = null;

		self.evPostActorCallback(600, RemoveActor);
		
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
			self.seqSpawn(self.seqName, self.seqID, self.seqCallback);
		}
		if (self.callback != null)
		{
			self.evPostActorCallback(0, self.callback);
		}
		if (self.spawnsound != 0)
		{
			self.play3DSound(self.spawnsound, 0, 0);
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
		callback fxFlareSpark;
		spawnsound "FLARAIR2";
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
		callback fxTeslaAlt;
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
		callback fxFlareSpark;
		spawnsound "FLARAIR2";
	}
		
	override void initMissile(BloodActor spawner)
	{
		super.initMissile(spawner);
		self.evPostActorCallback(30, FlareBurst);
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
		self.seqSpawn(self.seqName, self.seqID + index, nullptr);
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
		seqID 22;
		seqCallback FireballSeqCallback;
		spawnsound "SPRAYFIR";
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
		spawnsound "ARC2";
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
		spawnsound "SKULAIR4";
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
		spawnsound "ARC3";
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
		seqID 61;
		seqCallback NapalmSeqCallback;
		spawnsound "SPRAYFIR";
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
		seqCallback Fx32Callback;
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
		seqCallback Fx33Callback;
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
		callback fxFlameLick;
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
		callback fxArcSpark;
	}
}


