
class RedneckBillyRay : DukeActor
{
	const BILLYRAYSTRENGTH = 100;
	default
	{
		pic "BILLYRAY";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		precacheclass "RedneckBillyJibA", "RedneckBillyJibB";
		Strength BILLYRAYSTRENGTH;
		
	}
	
	override void PlayFTASound(int mode)
	{
		if (mode == 0) self.PlayActorSound("BR_RECOG");
		else self.PlayActorSound("FART1");
	}
	
	override void Initialize()
	{
		self.scale = (0.390625, 0.328125);
		self.setClipDistFromTile();
	}
	
	
}

class RedneckBillyRayStayput: RedneckBillyRay
{
	default
	{
		pic "BILLYRAYSTAYPUT";
		+BADGUYSTAYPUT
	}

	override void PlayFTASound(int mode)
	{
	}

}