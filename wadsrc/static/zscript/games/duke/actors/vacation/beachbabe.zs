class VacationBeachBabe : DukeLizmanJump
{

	const BEACHBABESTRENGTH = 30;
	default
	{
		pic "BEACHBABE";
		Strength BEACHBABESTRENGTH;
		+INTERNAL_BADGUY;
	}
	
	override void PlayFTASound()
	{
		self.PlayActorSound("CAPT_RECOG");
	}
}