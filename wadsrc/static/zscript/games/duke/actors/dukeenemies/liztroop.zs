
class DukeLizTrooper : DukeActor
{
	const TROOPSTRENGTH = 30;
	
	default
	{
		pic "LIZTROOP";
		Strength TROOPSTRENGTH;
		precacheclass "DukeHeadJib1", "DukeArmJib1", "DukeLegJib1";
		
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+GREENSLIMEFOOD;
		+TRANSFERPALTOJIBS;
		+DONTENTERWATERONGROUND;
		
	}
	
	override void Initialize()
	{
		if (self.pal == 0 || self.pal == 2) self.pal = 22;
	}
	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("PRED_RECOG");
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperToilet : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPONTOILET";
		StartAction "none";
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperSitting : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPJUSTSIT";
		StartAction "none";
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperShoot : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPSHOOT";
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperJetpack : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPJETPACK";
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperDucking : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPDUCKING";
	}
	
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperRunning : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPRUNNING";
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeLizTrooperStayput : DukeLizTrooper
{
	default
	{
		pic "LIZTROOPSTAYPUT";
		+BADGUYSTAYPUT;
	}
	
	override void PlayFTASound(int mode)
	{
	}
	
	
}
