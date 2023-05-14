class DukeFemBase : DukeActor
{
	meta int femflags;
	property femflags: femflags;
	
	default
	{
		+TRIGGERRESPAWN;
		+HITRADIUSCHECK;
		//StartAction "FEMANIMATE";
		Strength TOUGH;
	}		

	const GROWSCRAP = 1;
	const SLOWANIM = 2;
	const TOUGHANIM = 4;
	const TIPME = 8;
	const KILLME = 16;
	const FEMDANCE = 32;
	const FREEZEANIM2 = 64;
	

	override void Initialize(DukeActor spawner)
	{
		self.yint = self.hitag;
		self.hitag = -1;
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
		self.ChangeStat(STAT_ZOMBIEACTOR);
	}
	
}


class DukeBloodyPole : DukeFemBase
{
	default
	{
		pic "BLOODYPOLE";
		-TRIGGERRESPAWN;
		-HITRADIUSCHECK;
		StartAction 'none';
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 8;
		self.ChangeStat(STAT_ZOMBIEACTOR);
	}
	
	
}

class DukeFemale1 : DukeFemBase
{
	default
	{
		pic "FEM1";
		DukeFemBase.femflags TIPME | FEMDANCE;
	}

}
		
class DukeFemale2 : DukeFemBase
{
	default
	{
		pic "FEM2";
		DukeFemBase.femflags TIPME;
	}
	
}
	
class DukeFemale3 : DukeFemBase
{
	default
	{
		pic "FEM3";
		DukeFemBase.femflags TIPME;
	}
}
	
class DukeFemale4 : DukeFemBase
{
	default
	{
		pic "FEM4";
	}
}
	
class DukeFemale5 : DukeFemBase
{
	default
	{
		pic "FEM5";
		DukeFemBase.femflags FREEZEANIM2 | KILLME;
	}
}
	
class DukeFemale6 : DukeFemBase
{
	default
	{
		pic "FEM6";
		+NOGRAVITY;
		DukeFemBase.femflags FREEZEANIM2 | KILLME;
	}
	

}
	
class DukeFemale7 : DukeFemBase
{
	default
	{
		pic "FEM7";
		DukeFemBase.femflags TIPME;
	}
	
}
	
class DukeFemale8 : DukeFemBase
{
	default
	{
		pic "FEM8";
		DukeFemBase.femflags FREEZEANIM2;
	}
	

}
	
class DukeFemale9 : DukeFemBase
{
	default
	{
		pic "FEM9";
		DukeFemBase.femflags FREEZEANIM2;
	}
}
	
class DukeFemale10 : DukeFemBase
{
	default
	{
		pic "FEM10";
		DukeFemBase.femflags SLOWANIM | FREEZEANIM2 | TIPME;
	}
}
	
class DukeNaked : DukeFemBase
{
	default
	{
		pic "NAKED1";
		-HITRADIUSCHECK;
		+NOGRAVITY;
		DukeFemBase.femflags GROWSCRAP | FREEZEANIM2 | KILLME;
	}
}
	
class DukeToughGal : DukeFemBase
{
	const MANWOMANSTRENGTH = 100;
	default
	{
		pic "TOUGHGAL";
		-HITRADIUSCHECK;
		Strength MANWOMANSTRENGTH;
		DukeFemBase.femflags TOUGHANIM | FREEZEANIM2;
	}
	
}

class DukePodFemale : DukeFemBase
{
	default
	{
		pic "PODFEM1";
		DukeFemBase.femflags GROWSCRAP | FREEZEANIM2 | KILLME;
	}
	
	override void Initialize(DukeActor spawner)
	{
		Super.Initialize(spawner);
		self.extra <<= 1;
	}
	
}

class DukeFem6Pad: DukeActor
{
	default
	{
		pic "FEM6PAD";
	}
}
