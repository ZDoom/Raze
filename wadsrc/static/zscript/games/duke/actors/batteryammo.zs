class DukeBatteryAmmo : DukeActor
{
	default
	{
		pic "BATTERYAMMO";
	}
	
	override void Initialize()
	{
		commonItemSetup();
	}
}

class RedneckRifleAmmo : DukeActor
{
	default
	{
		pic "BATTERYAMMO";
	}
	
	override void Initialize()
	{
		commonItemSetup((0.234375, 0.234375));
	}
}
