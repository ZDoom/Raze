class NWinterWoodSlats : DukeScriptedBreakable // WOODSLATS (3757)
{
	default
	{
		pic "WOODSLATS";
		Strength MEDIUMSTRENGTH;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.PlayActorSound("GLASS_HEAVYBREAK");
		self.spawndebris(DukeScrap.Scrap3, 5);
		self.spawndebris(DukeScrap.Scrap4, 3);
	}
}
class NWinterRibbon : DukeScriptedBreakable // RIBBON (930)
{
	default
	{
		pic "RIBBON";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.PlayActorSound("GLASS_HEAVYBREAK");
		self.spawndebris(DukeScrap.Scrap3, 5);
		self.spawndebris(DukeScrap.Scrap4, 3);
	}
}
class NWinterSSpeaker : DukeScriptedBreakable // SSPEAKER (589)
{
	default
	{
		pic "SSPEAKER";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.spawndebris(DukeScrap.Scrap4, 3);
	}
}
class NWinterMybox : NWinterRibbon // MYBOX (3666)
{
	default
	{
		pic "MYBOX";
		Strength WEAK;
	}
}
class NWinterDrunkElf : DukeScriptedBreakable // DRUNKELF (3592)
{
	default
	{
		pic "DRUNKELF";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.PlayActorSound("PRED_DYING");
		self.spawnguts('DukeJibs6', 2);
		self.spawnguts('DukeJibs6', 3);
		self.spawnguts('DukeJibs6', 4);
	}
}
class NWinterHalfBitch : NWinterDrunkElf // HALFBITCH (3627)
{
	default
	{
		pic "HALFBITCH";
		Strength WEAK;
	}
}
class NWinterSlutAss : NWinterDrunkElf // SLUTASS (3637)
{
	default
	{
		pic "SLUTASS";
		Strength WEAK;
	}
}
class NWinterChoochooslut : DukeScriptedBreakable // CHOOCHOOSLUT (3779)
{
	default
	{
		pic "CHOOCHOOSLUT";
		Strength WEAK;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.PlayActorSound("GLASS_HEAVYBREAK");
		self.spawndebris(DukeScrap.Scrap3, 5);
		self.spawndebris(DukeScrap.Scrap4, 3);
	}
}
class NWinterHorsepower : NWinterChoochooslut // HORSEPOWER (3783)
{
	default
	{
		pic "HORSEPOWER";
		Strength WEAK;
	}
}
class NWinterSnowgib : DukeScriptedBreakable // SNOWGIB (3777)
{
	default
	{
		pic "SNOWGIB";
		Strength WEAK;
	}
}
class NWinterTreeWithSomething : DukeScriptedBreakable // TREEWITHSOMETHING (3648)
{
	default
	{
		pic "TREEWITHSOMETHING";
		Strength TOUGH;
	}
	
	override void BrkKilled(DukePlayer p, double pdist)
	{
		self.PlayActorSound("GLASS_HEAVYBREAK");
		self.spawndebris(DukeScrap.Scrap3, 5);
		if (Duke.rnd(96))
		{
			if (Duke.rnd(64))
			{
				self.spawn('DukeAtomicHealth');
			}
			else
			{
				self.spawn('DukeShield');
			}
		}
		else
		{
			if (Duke.rnd(128))
			{
				self.spawn('DukeSteroids');
			}
			else
			{
				self.spawn('DukeFeces');
			}
		}
	}
}
