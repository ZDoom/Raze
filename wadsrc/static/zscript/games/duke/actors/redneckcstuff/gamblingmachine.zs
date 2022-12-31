
class RedneckGamblingMachine : DukeActor
{
	default
	{
		spriteset "GAMBLINGMACHINE2", "GAMBLINGMACHINE3", "GAMBLINGMACHINE4", "GAMBLINGMACHINE5", "GAMBLINGMACHINE6", "GAMBLINGMACHINE7", "GAMBLINGMACHINE8", "GAMBLINGMACHINEBROKE";
		spritesetindex 3;
		RedneckGamblingMachine.winsound "COW3";

	}
	
	meta Sound winsound;
	property winsound: winsound;
	
	override void Initialize()
	{
		self.lotag = 1;
		self.clipdist = 0;
		self.extra = 0;
		self.ChangeStat(STAT_ACTOR);
	}

	override void Tick()
	{
		int spriteindex;
		if (self.extra && self.spritesetindex < 7)
		{
			let pl = Duke.GetViewPlayer();
			
			if (self.spritesetindex != 0)
				spriteindex = 0;
			self.extra--;
			if (self.extra == 0)
			{
				int rvar = random(0, 127);
				if (rvar < 96)
				{
					spriteindex = 3;
				}
				else if (rvar < 112)
				{
					if (pl.SlotWin & 1)
					{
						spriteindex = 3;
					}
					else
					{
						spriteindex = 2;
						self.spawn("RedneckRifleAmmo");
						pl.SlotWin |= 1;
						self.PlayActorSound(winsound);
					}
				}
				else if (rvar < 120)
				{
					if (pl.SlotWin & 2)
					{
						spriteindex = 3;
					}
					else
					{
						spriteindex = 6;
						self.spawn("RedneckDynamite");
						pl.SlotWin |= 2;
						self.PlayActorSound(winsound);
					}
				}
				else if (rvar < 126)
				{
					if (pl.SlotWin & 4)
					{
						spriteindex = 3;
					}
					else
					{
						spriteindex = 5;
						self.spawn("RedneckPorkRinds");
						pl.SlotWin |= 4;
						self.PlayActorSound(winsound);
					}
				}
				else
				{
					if (pl.SlotWin & 8)
					{
						spriteindex = 3;
					}
					else
					{
						spriteindex = 4;
						spawn("RedneckGoogooCluster");
						pl.SlotWin |= 8;
						self.PlayActorSound(winsound);
					}
				}
			}
			self.setSpriteSetImage(spriteindex);
		}
	}
	
	override void OnHit(DukeActor proj)
	{
		if (self.spritesetindex < 7)
		{
			self.SetSpriteSetImage(7);
			self.PlayActorSound("GLASS_HEAVYBREAK");
		}
	}

	override bool OnUse(DukePlayer p)
	{
		if (self.spritesetindex > 1 && self.spritesetindex < 7)
		{
			self.extra = 60;
			self.PlayActorSound("SLOTS");
			return true;
		}
		return false;
	}
	
}

class RedneckGamblingMachine2 : RedneckGamblingMachine
{
	default
	{
		spriteset "GAMBLINGMACHINE2_2", "GAMBLINGMACHINE2_3", "GAMBLINGMACHINE2_4", "GAMBLINGMACHINE2_5", "GAMBLINGMACHINE2_6", "GAMBLINGMACHINE2_7", "GAMBLINGMACHINE2_8", "GAMBLINGMACHINE2_BROKE";
		RedneckGamblingMachine.winsound "VX_TPIN2";
	}
}
