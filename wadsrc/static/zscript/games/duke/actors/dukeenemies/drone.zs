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
		+NOSHOTGUNBLOOD;
		falladjustz 0;
		floating_floordist 30;
		floating_ceilingdist 50;
		
	}
	
	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("DRON_RECOG");
	}

}
