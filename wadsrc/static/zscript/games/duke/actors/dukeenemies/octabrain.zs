class DukeOctabrain : DukeActor
{
	const OCTASTRENGTH = 175;
	const OCTASCRATCHINGPLAYER = -11;
	
	default
	{
		pic "OCTABRAIN";
		Strength OCTASTRENGTH;
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NOWATERDIP;
		falladjustz 0;
		
	}
	
	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("OCTA_RECOG");
	}

}

class DukeOctabrainStayput: DukeOctabrain
{
	default
	{
		pic "OCTABRAINSTAYPUT";
		+DONTDIVEALIVE;
		+BADGUYSTAYPUT;
	}
	

}

