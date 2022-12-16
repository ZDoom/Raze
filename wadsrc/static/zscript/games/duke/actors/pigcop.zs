class DukePigCop : DukeActor
{
	default
	{
		pic "PIGCOP";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+GREENSLIMEFOOD;
	}
	
	override void PlayFTASound()
	{
		self.PlayActorSound("PIG_RECOG");
	}
}

class DukePigCopStayput: DukePigCop
{
	default
	{
		pic "PIGCOPSTAYPUT";
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
}

class DukePigCopDive : DukePigCopStayput
{
	default
	{
		pic "PIGCOPDIVE";
	}
	
	override void PlayFTASound()
	{
	}
}
