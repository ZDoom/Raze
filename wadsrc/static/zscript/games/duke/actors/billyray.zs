
class RedneckBillyRay : DukeActor
{
	default
	{
		pic "BILLYRAY";
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

class RedneckBillyRayCock: RedneckBillyRay
{
	default
	{
		pic "BILLYCOCK";
	}

}

class RedneckBillyRaySniper: RedneckBillyRay
{
	default
	{
		pic "BRAYSNIPER";
	}

	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
}
