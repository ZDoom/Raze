
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
	
	override bool animate(tspritetype t)
	{
		if (Raze.isRRRA())
		{
			/* todo: define this in the animation.
			if (t->picnum >= RTILE_BILLYRAY + 5 && t->picnum <= RTILE_BILLYRAY + 9)
				t->shade = -127;
			*/
		}
		return false;
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
