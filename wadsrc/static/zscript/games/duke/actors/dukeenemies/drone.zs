class DukeDrone : DukeActor
{
	const DRONESTRENGTH = 150;
	
	default
	{
		pic "DRONE";
		Strength DRONESTRENGTH;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NOWATERDIP;
		+FLOATING;
		+QUICKALTERANG;
		+NOJIBS;
		+NOHITJIBS;
		falladjustz 0;
		floating_floordist 30;
		floating_ceilingdist 50;
		
	}
	
	override void PlayFTASound()
	{
		self.PlayActorSound("DRON_RECOG");
	}

}
