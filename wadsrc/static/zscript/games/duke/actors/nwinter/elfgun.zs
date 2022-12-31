
class NWinterElfGun : DukeActor
{
	const ELFGUNSTRENGTH = 75;

	default
	{
		+BADGUY
		+KILLCOUNT
		Strength ELFGUNSTRENGTH;
		
		pic "ELFGUN";
	}
	
	
}


class NWinterElfGunStayput : NWinterElfGun
{
	default
	{
		+BADGUYSTAYPUT
	}
	
	
}
