class DukeCommander : DukeActor
{
	const COMMANDERSTRENGTH = 350;
	const CAPTSPINNINGPLAYER = -11;
	
	default
	{
		pic "COMMANDER";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NOWATERDIP;
		+FLOATING;
		+SHOOTCENTERED;
		+NORANDOMANGLEWHENBLOCKED;
		+NOHITJIBS;
		gutsoffset -24;
		falladjustz 0;
		floating_floordist 8;
		floating_ceilingdist 80;
		Strength COMMANDERSTRENGTH;
		
	}
	
	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("COMM_RECOG");
	}

	
}

class DukeCommanderStayput: DukeCommander
{
	default
	{
		pic "COMMANDERSTAYPUT";
		+BADGUYSTAYPUT;
	}

}
