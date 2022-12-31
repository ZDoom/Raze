


class DukeWaterFountain : DukeActor
{
	default
	{
		spriteset "WATERFOUNTAIN", "WATERFOUNTAIN1", "WATERFOUNTAIN2", "WATERFOUNTAIN3", "WATERFOUNTAINBROKE";
	}
	
	//---------------------------------------------------------------------------
	//
	// RR implements this but the sprites are empty.
	//
	//---------------------------------------------------------------------------

	override void Initialize()
	{
		self.setSpritesetImage(0);
		self.lotag = 1;
		self.cstat = CSTAT_SPRITE_BLOCK_ALL; // Make it hitable
		self.extra = 1;
		self.ChangeStat(STAT_STANDABLE);
	}
	
	override void Tick()
	{
		if (self.counter > 0 && self.spritesetindex < 4)
		{
			int frame = self.spritesetindex;
			if (self.counter < 20)
			{
				self.counter++;

				frame++;

				if (frame == 3)
					frame = 1;

				self.setSpritesetImage(frame);
			}
			else
			{
				let p = self.findplayer();
				// this does not really work, but fixing this will probably draw complaints for not being authentic.
				if ((self.pos - p.actor.pos.plusZ(28)).Sum() > 32)
				{
					self.counter = 0;
					self.setSpritesetImage(0);
				}
				else self.counter = 1;
			}
		}
	}
	
	override void onHit(DukeActor hitter)
	{
		if (self.spritesetindex < 4)
		{
			self.setSpritesetImage(4);
			self.spawn("DukeToiletWater");
		}
		else
		{
			self.PlayActorSound("GLASS_BREAKING");
			self.angle = FRandom(0., 360.);
			self.lotsofglass(8);
			self.Destroy();
		}
	}

	override bool onUse(DukePlayer user)
	{
		if (self.counter != 1)
		{
			self.counter = 1;
			let act = user.actor;
			self.ownerActor = act;

			if (act.extra < gs.max_player_health)
			{
				act.extra++;
				act.PlayActorSound("PLAYER_DRINKING");
			}
		}
		return true;
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeWaterFountainBroke : DukeActor
{
	default
	{
		pic "WATERFOUNTAINBROKE";
	}

	override void Initialize()
	{
		self.ChangeStat(STAT_STANDABLE);
	}
	
	override void onHit(DukeActor hitter)
	{
		self.PlayActorSound("GLASS_BREAKING");
		self.angle = FRandom(0., 360.);
		self.lotsofglass(8);
		self.Destroy();
	}
}
