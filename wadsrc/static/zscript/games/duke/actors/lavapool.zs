class DukeLavaPool : DukeActor
{
	default
	{
		pic "LAVAPOOL";
		+FORCERUNCON;
	}
	
	override void Initialize()
	{
		bool away = self.isAwayFromWall(6.75);
		
		if (!away)
		{
			self.ChangeStat(STAT_MISC);
			self.scale = (0, 0); 
			return;
		}

		if (self.sector.lotag == 1)
		{
			return;
		}
	

		self.cstat |= CSTAT_SPRITE_ALIGNMENT_FLOOR;
		double c, f;
		[c, f] = self.sector.getslopes(self.pos.XY);
		self.pos.Z = f - 0.78125;
		if (self != self.ownerActor)
			self.scale = (REPEAT_SCALE, REPEAT_SCALE);
		self.ChangeStat(STAT_MISC);
	}
}

class DukeLavaPoolBubble : DukeActor
{
	default
	{
		pic "LAVAPOOLBUBBLE";
		+FORCERUNCON;
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;
		self.ChangeStat(STAT_MISC);
		if (owner.scale.X < 0.46875)
		{
			self.scale = (0, 0); 
			return;
		}
		self.pos.X += frandom(-16, 16);
		self.pos.Y += frandom(-16, 16);
		self.scale = (0.25, 0.25);
	}	
}

