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
		+BADGUYSTAYPUT;
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

