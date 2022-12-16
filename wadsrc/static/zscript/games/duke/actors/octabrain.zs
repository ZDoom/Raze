class DukeOctabrain : DukeActor
{
	default
	{
		pic "OCTABRAIN";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+NOWATERDIP;
	}
	
	override void PlayFTASound()
	{
		self.PlayActorSound("OCTA_RECOG");
	}
}

class DukeOctabrainStayput: DukeOctabrain
{
	default
	{
		pic "OCTABRAINSTAYPUT";
		+DONTDIVEALIVE;
	}
	
	override void initialize()
	{
		super.initialize();
		self.actorstayput = self.sector;	// make this a flag once everything has been exported.
	}
}
