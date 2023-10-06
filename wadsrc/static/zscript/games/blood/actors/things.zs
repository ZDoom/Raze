class BloodThingBase : BloodActor
{
	meta int defhealth;
	meta int mass;
	meta int defflags;
	meta int bouncefactor;
	meta int dmgResist;
	meta int defcstat;

	property prefix: none;
	property health: defhealth;
	property mass: mass;
	property flags: defflags;
	property bouncefactor: bouncefactor;
	property dmgResist: dmgResist;
	property cstat: defcstat;
}

class BloodThingTNTBarrel : BloodThingBase
{
	default
	{
		health 25;
		mass 250;
		clipdist 8.000000;
		flags 11;
		bouncefactor 4096;
		dmgResist 80;
		cstat CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "TNTBarrel";
		dmgcontrol 256, 256, 128, 64, 0, 0, 128;
	}
}

class BloodThingArmedProxBomb : BloodThingBase
{
	default
	{
		health 5;
		mass 5;
		clipdist 4.000000;
		flags 3;
		bouncefactor 24576;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "ProxBomb";
		shade -16;
		scale 0.500000, 0.500000;
		dmgcontrol 256, 256, 256, 64, 0, 0, 512;
	}
}

class BloodThingArmedRemoteBomb : BloodThingBase
{
	default
	{
		health 5;
		mass 5;
		clipdist 4.000000;
		flags 3;
		bouncefactor 24576;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "ArmedRemote";
		shade -16;
		scale 0.500000, 0.500000;
		dmgcontrol 256, 256, 256, 64, 0, 0, 512;
	}
}

class BloodThingVase1 : BloodThingBase
{
	default
	{
		health 1;
		mass 20;
		clipdist 8.000000;
		flags 3;
		bouncefactor 32768;
		dmgResist 80;
		pic "Vase1";
		dmgcontrol 256, 0, 256, 128, 0, 0, 0;
	}
}

class BloodThingVase2 : BloodThingBase
{
	default
	{
		health 1;
		mass 150;
		clipdist 8.000000;
		flags 3;
		bouncefactor 32768;
		dmgResist 80;
		pic "Vase2";
		dmgcontrol 256, 256, 256, 128, 0, 0, 0;
	}
}

class BloodThingCrateFace : BloodThingBase
{
	default
	{
		health 10;
		clipdist 0.000000;
		pic "CreateFace";
		dmgcontrol 0, 0, 0, 256, 0, 0, 0;
	}
}

class BloodThingGlassWindow : BloodThingBase
{
	default
	{
		health 1;
		clipdist 0.000000;
		pic "GlassWindow";
		dmgcontrol 256, 0, 256, 256, 0, 0, 0;
	}
}

class BloodThingFluorescent : BloodThingBase
{
	default
	{
		health 1;
		clipdist 0.000000;
		pic "Fluorescent";
		dmgcontrol 256, 0, 256, 256, 0, 0, 512;
	}
}

class BloodThingWallCrack : BloodThingBase
{
	default
	{
		health 50;
		clipdist 0.000000;
		pic "WallCrack";
		dmgcontrol 0, 0, 0, 256, 0, 0, 0;
	}
}

class BloodThingWoodBeam : BloodThingBase
{
	default
	{
		health 8;
		clipdist 0.000000;
		pic "WoodBeam";
		dmgcontrol 256, 0, 256, 128, 0, 0, 0;
	}
}

class BloodThingSpiderWeb : BloodThingBase
{
	default
	{
		health 4;
		clipdist 0.000000;
		pic "SpiderWeb";
		dmgcontrol 256, 256, 64, 256, 0, 0, 128;
	}
}

class BloodThingMetalGrate : BloodThingBase
{
	default
	{
		health 40;
		clipdist 0.000000;
		pic "MetalGrate";
		dmgcontrol 64, 0, 128, 256, 0, 0, 0;
	}
}

class BloodThingFlammableTree : BloodThingBase
{
	default
	{
		health 1;
		clipdist 0.000000;
		dmgcontrol 0, 256, 0, 256, 0, 0, 128;
	}
}

class BloodTrapMachinegun : BloodThingBase
{
	default
	{
		health 1000;
		clipdist 0.000000;
		flags 8;
		dmgcontrol 0, 0, 128, 256, 0, 0, 512;
	}
}

class BloodThingFallingRock : BloodThingBase
{
	default
	{
		mass 15;
		clipdist 2.000000;
		flags 3;
		bouncefactor 32768;
	}
}

class BloodThingKickablePail : BloodThingBase
{
	default
	{
		mass 8;
		clipdist 12.000000;
		flags 3;
		bouncefactor 49152;
	}
}

class BloodThingObjectGib : BloodThingBase
{
	default
	{
		health 10;
		mass 2;
		clipdist 0.000000;
		bouncefactor 32768;
		dmgcontrol 256, 0, 256, 256, 0, 0, 128;
	}
}

class BloodThingObjectExplode : BloodThingBase
{
	default
	{
		health 20;
		mass 2;
		clipdist 0.000000;
		bouncefactor 32768;
		dmgcontrol 0, 0, 0, 256, 0, 0, 128;
	}
}

class BloodThingArmedTNTStick : BloodThingBase
{
	default
	{
		health 5;
		mass 14;
		clipdist 4.000000;
		flags 3;
		bouncefactor 24576;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "ArmedTNT";
		shade -32;
		scale 0.500000, 0.500000;
		dmgcontrol 64, 256, 128, 64, 0, 0, 256;
	}
}

class BloodThingArmedTNTBundle : BloodThingBase
{
	default
	{
		health 5;
		mass 14;
		clipdist 4.000000;
		flags 3;
		bouncefactor 24576;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "ArmedTNTBundle";
		shade -32;
		scale 0.500000, 0.500000;
		dmgcontrol 64, 256, 128, 64, 0, 0, 256;
	}
}

class BloodThingArmedSpray : BloodThingBase
{
	default
	{
		health 5;
		mass 14;
		clipdist 4.000000;
		flags 3;
		bouncefactor 32768;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "ArmedSpray";
		shade -128;
		scale 0.500000, 0.500000;
		dmgcontrol 64, 256, 128, 64, 0, 0, 256;
	}
}

class BloodThingBone : BloodThingBase
{
	default
	{
		health 5;
		mass 6;
		clipdist 4.000000;
		flags 3;
		bouncefactor 32768;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "Bone";
		scale 0.500000, 0.500000;
	}
}

class BloodThing422 : BloodThingBase
{
	default
	{
		health 8;
		mass 3;
		clipdist 4.000000;
		flags 11;
		bouncefactor 32768;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		dmgcontrol 256, 0, 256, 256, 0, 0, 0;
	}
}

class BloodThingDripWater : BloodThingBase
{
	default
	{
		mass 1;
		clipdist 0.250000;
		flags 2;
		pic "DripWater";
		pal 10;
	}
}

class BloodThingDripBlood : BloodThingBase
{
	default
	{
		mass 1;
		clipdist 0.250000;
		flags 2;
		pic "DripBlood";
		pal 2;
	}
}

class BloodThingBloodBits : BloodThingBase
{
	default
	{
		health 15;
		mass 4;
		clipdist 1.000000;
		flags 3;
		bouncefactor 24576;
		cstat CSTAT_SPRITE_BLOCK_ALL;
		dmgcontrol 128, 64, 256, 256, 0, 0, 256;
	}
}

class BloodThingBloodChunks : BloodThingBase
{
	default
	{
		health 30;
		mass 30;
		clipdist 2.000000;
		flags 3;
		bouncefactor 8192;
		cstat CSTAT_SPRITE_BLOCK_ALL;
		dmgcontrol 128, 64, 256, 256, 0, 0, 64;
	}
}

class BloodThingZombieHead : BloodThingBase
{
	default
	{
		health 60;
		mass 5;
		clipdist 8.000000;
		flags 3;
		bouncefactor 40960;
		dmgResist 1280;
		cstat CSTAT_SPRITE_BLOCK_ALL;
		pic "ZombieHead";
		scale 0.625000, 0.625000;
		dmgcontrol 128, 64, 256, 256, 0, 0, 64;
	}
}

class BloodThingNapalmBall : BloodThingBase
{
	default
	{
		health 80;
		mass 30;
		clipdist 8.000000;
		flags 3;
		bouncefactor 57344;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "NapalmBall";
		shade -128;
		scale 0.500000, 0.500000;
	}
}

class BloodThingPodFireBall : BloodThingBase
{
	default
	{
		health 80;
		mass 30;
		clipdist 8.000000;
		flags 3;
		bouncefactor 57344;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "PodFireball";
		shade -128;
		scale 0.500000, 0.500000;
		dmgcontrol 256, 0, 256, 256, 0, 0, 0;
	}
}

class BloodThingPodGreenBall : BloodThingBase
{
	default
	{
		health 80;
		mass 30;
		clipdist 8.000000;
		flags 3;
		bouncefactor 57344;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "PodGreenBall";
		shade -128;
		scale 0.500000, 0.500000;
		dmgcontrol 256, 0, 256, 256, 0, 0, 0;
	}
}

class BloodThingDroppedLifeLeech : BloodThingBase
{
	default
	{
		health 150;
		mass 30;
		clipdist 12.000000;
		flags 3;
		bouncefactor 32768;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_ALL;
		pic "ICONLEECH";
		shade -128;
		scale 0.750000, 0.750000;
		dmgcontrol 64, 64, 112, 64, 0, 96, 96;
	}
}

class BloodThingVoodooHead : BloodThingBase
{
	default
	{
		health 1;
		mass 30;
		clipdist 12.000000;
		flags 3;
		bouncefactor 32768;
		dmgResist 1600;
		pic "VoodooHead";
		shade -128;
		scale 0.250000, 0.250000;
	}
}

class BloodThingTNTProx : BloodThingBase
{
	default
	{
		health 5;
		mass 5;
		clipdist 4.000000;
		flags 3;
		bouncefactor 24576;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "ProxBomb";
		shade -16;
		pal 7;
		scale 0.500000, 0.500000;
		dmgcontrol 256, 256, 256, 64, 0, 0, 512;
	}
}

class BloodThingThrowableRock : BloodThingBase
{
	default
	{
		health 5;
		mass 6;
		clipdist 4.000000;
		flags 3;
		bouncefactor 32768;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_HITSCAN;
		pic "Bone";
		scale 0.500000, 0.500000;
	}
}

class BloodThingEnemyLifeLeech : BloodThingBase
{
	default
	{
		health 150;
		mass 30;
		clipdist 12.000000;
		flags 3;
		bouncefactor 32768;
		dmgResist 1600;
		cstat CSTAT_SPRITE_BLOCK_ALL;
		pic "ICONLEECH";
		shade -128;
		scale 0.687500, 0.687500;
		dmgcontrol 0, 1024, 512, 1024, 0, 64, 512;
	}
}

// traps
class BloodTrapFlame : BloodActor
{
}

class BloodTrapSawCircular : BloodActor
{
}

class BloodTrapZapSwitchable : BloodActor
{
}

class BloodTrapExploder : BloodActor
{
}

