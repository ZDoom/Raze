
class DukeLizMan : DukeActor
{
	default
	{
		pic "LIZMAN";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+GREENSLIMEFOOD;
		+DONTENTERWATER;
		+RANDOMANGLEONWATER;
		moveclipdist 18.25;
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
		+BADGUYSTAYPUT;
	}
	
	override void PlayFTASound()
	{
	}

}

