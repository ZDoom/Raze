
class RedneckBillyRay : DukeActor
{
	const BILLYRAYSTRENGTH = 100;
	default
	{
		pic "BILLYRAY";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		Strength BILLYRAYSTRENGTH;
		
	}
	
	override void PlayFTASound()
	{
		self.PlayActorSound("BR_RECOG");
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

	override void PlayFTASound()
	{
	}

}