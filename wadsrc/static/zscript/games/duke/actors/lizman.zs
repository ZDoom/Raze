
class DukeLizMan : DukeActor
{
	default
	{
		pic "LIZMAN";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+GREENSLIMEFOOD;
	}
	
	override void PlayFTASound()
	{
		self.PlayActorSound("CAPT_RECOG");
	}
}

class DukeLizManSpitting : DukeLizMan
{
	default
	{
		pic "LIZMANSPITTING";
	}
}

class DukeLizManFeeding : DukeLizMan
{
	default
	{
		pic "LIZMANFEEDING";
	}
}

class DukeLizManJump : DukeLizMan
{
	default
	{
		pic "LIZMANJUMP";
	}
}

class DukeLizManStayput : DukeLizMan
{
	default
	{
		pic "LIZMANSTAYPUT";
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

