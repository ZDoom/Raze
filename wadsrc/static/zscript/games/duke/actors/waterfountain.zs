


class DukeWaterFountain : DukeActor
{
	default
	{
		spriteset "WATERFOUNTAIN", "WATERFOUNTAIN1", "WATERFOUNTAIN2", "WATERFOUNTAIN3", "WATERFOUNTAINBROKE";
		statnum STAT_STANDABLE;
	}
	
	//---------------------------------------------------------------------------
	//
	// RR implements this but the sprites are empty.
	//
	//---------------------------------------------------------------------------

	override void Initialize()
	{
		self.setSpritePic(0);
		self.lotag = 1;
		self.cstat = CSTAT_SPRITE_BLOCK_ALL; // Make it hitable
		self.extra = 1;
	}
	
	override void Tick()
	{
		if (self.temp_data[0] > 0 && self.spritesetindex < 4)
		{
			int frame = self.spritesetindex;
			if (self.temp_data[0] < 20)
			{
				self.temp_data[0]++;

				frame++;

				if (frame == 3)
					frame = 1;

				self.setSpritePic(frame);
			}
			else
			{
				let p = self.findplayer();
				// this does not really work, but fixing this will probably draw complaints for not being authentic.
				if ((self.pos - p.actor.pos.plusZ(28)).Sum() > 32)
				{
					self.temp_data[0] = 0;
					self.setSpritePic(0);
				}
				else self.temp_data[0] = 1;
			}
		}
	}
	
	override void onHit(DukeActor hitter)
	{
		if (self.spritesetindex < 4)
		{
			self.setSpritePic(4);
			self.spawn("DukeToiletWater");
		}
		else
		{
			self.PlayActorSound(DukeSnd.GLASS_BREAKING);
			self.angle = FRandom(0., 360.);
			self.lotsofglass(8);
			self.Destroy();
		}
	}

	override void onUse(DukePlayer user)
	{
		if (self.temp_data[0] != 1)
		{
			self.temp_data[0] = 1;
			let act = user.actor;
			self.ownerActor = act;

			if (act.extra < gs.max_player_health)
			{
				act.extra++;
				act.PlayActorSound(DukeSnd.DUKE_DRINKING);
			}
		}
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
		statnum STAT_STANDABLE;
	}

	override void onHit(DukeActor hitter)
	{
		self.PlayActorSound(DukeSnd.GLASS_BREAKING);
		self.angle = FRandom(0., 360.);
		self.lotsofglass(8);
		self.Destroy();
	}
}
