
class DukeNukeButton : DukeActor
{
	default
	{
		spriteset "NUKEBUTTON", "NUKEBUTTON1",  "NUKEBUTTON2",  "NUKEBUTTON3";
	}
	
	override void Initialize()
	{
		self.ChangeStat(STAT_MISC);
	}
	
	override void Tick()
	{
		if (self.temp_data[0])
		{
			self.temp_data[0]++;
			let Owner = self.ownerActor;
			if (self.temp_data[0] == 8) self.setSpritesetImage(1);
			else if (self.temp_data[0] == 16 && Owner)
			{
				self.setSpritesetImage(2);
				Owner.GetPlayer().fist_incs = 1;
			}
			if (Owner && Owner.GetPlayer().fist_incs == 26)
				self.setSpritesetImage(3);
		}
	}
	
	override bool OnUse(DukePlayer p)
	{
		if (self.temp_data[0] == 0 && !p.hitablockingwall())
		{
			self.temp_data[0] = 1;
			self.ownerActor = p.actor;
			p.buttonpalette = self.pal;
			if (p.buttonpalette)
				ud.secretlevel = self.lotag;
			else ud.secretlevel = 0;
		}
		return true;
	}
}

