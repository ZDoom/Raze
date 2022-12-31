
class RedneckCheerleader : DukeActor
{
	default
	{
		pic "CHEER";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+LOOKALLAROUND;
		+ALTPROJECTILESPRITE; // owed to CON's shittiness. Todo: Think of something better.
		jumptoplayer_factor 1.6;
		Strength 200;
	}
	override void Initialize()
	{
		self.scale = (0.3125, 0.3125);
		self.setClipDistFromTile();
	}

}