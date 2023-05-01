
class DukeBlimp : DukeActor
{
	const BLIMPRESPAWNTIME = 2048;

	default
	{
		pic "BLIMP";
		Strength 1;
		+SPAWNWEAPONDEBRIS;
		action "BLIMPWAITTORESPAWN", 0;
	}
	
	override void Initialize()
	{
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL;
		self.clipdist = 32;
		self.ChangeStat(STAT_ACTOR);
	}
	
	void state_blimphitstate(DukePlayer p, double pdist)
	{
		self.cstat = 0;
		if (self.sector != null) self.spawn('DukeFirstgunSprite');
		if (self.sector != null) self.spawn('DukeExplosion2');
		self.spawndebris(DukeScrap.Scrap1, 40);
		self.spawndebris(DukeScrap.Scrap2, 32);
		self.spawndebris(DukeScrap.Scrap3, 32);
		self.spawndebris(DukeScrap.Scrap4, 32);
		self.spawndebris(DukeScrap.Scrap5, 32);
		self.PlayActorSound("PIPEBOMB_EXPLODE");
		if (ud.respawn_items) // for everything else:
		{
			setAction('BLIMPWAITTORESPAWN');
			self.counter = 0;
			self.cstat = CSTAT_SPRITE_INVISIBLE;
		}
		else
		{
			self.killit();
		}
	}
	
	override void RunState(DukePlayer p, double pdist)
	{
		if (self.curAction.name == 'BLIMPWAITTORESPAWN')
		{
			if (self.counter >= BLIMPRESPAWNTIME)
			{
				setAction('none');
				self.cstat = 0;
			}
			return;
		}
		if (self.ifhitbyweapon() >= 0)
		{
			if (self.attackertype.GetClassName() == 'DukeRadiusExplosion')
			{
				state_blimphitstate(p, pdist);
			}
			if (self.attackertype.GetClassName() == 'DukeRPG')
			{
				state_blimphitstate(p, pdist);
			}
			self.extra = 1;
		}
	}
}		
