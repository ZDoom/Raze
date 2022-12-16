
class DukeLizTrooper : DukeActor
{
	default
	{
		pic "LIZTROOP";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+GREENSLIMEFOOD;
		+TRANSFERPALTOJIBS;
	}
	
	override void Initialize()
	{
		Super.Initialize();
		if (pal == 0) pal = 22;
	}
	
	override void PlayFTASound()
	{
		self.PlayActorSound("PRED_RECOG");
	}
}

class DukeLizTrooperToilet : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPONTOILET";
	}
}

class DukeLizTrooperSitting : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPJUSTSIT";
	}
}

class DukeLizTrooperShoot : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPSHOOT";
	}
}

class DukeLizTrooperJetpack : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPJETPACK";
	}
}

class DukeLizTrooperDucking : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPDUCKING";
	}
}

class DukeLizTrooperRunning : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPRUNNING";
	}
}

class DukeLizTrooperStayput : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPSTAYPUT";
	}
	
	override void PlayFTASound()
	{
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
	
}
