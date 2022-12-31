


class DukeEgg : DukeActor
{
	default
	{
		pic "EGG";
		Strength TOUGH;
		
	}
	
	override void Initialize()
	{
		if (ud.monsters_off == 1)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
		}
		else
		{
			self.bINTERNAL_BADGUY = true; // the egg needs this flag, but it should not run through the monster init code.
			self.clipdist = 6;
			self.cstat = CSTAT_SPRITE_BLOCK_ALL | randomXFlip();
			self.ChangeStat(STAT_ZOMBIEACTOR);
		}
	}

}