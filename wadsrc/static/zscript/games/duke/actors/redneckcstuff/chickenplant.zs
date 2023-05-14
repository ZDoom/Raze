
class RedneckChickenSpawner1 : DukeActor
{
	default
	{
		pic "CHICKENASPAWN";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
		self.clipdist = 0;
		self.lotag = 0;
		self.ChangeStat(STAT_CHICKENPLANT);
	}
	
	override void Tick()
	{
		self.lotag--;
		if (self.lotag < 0)
		{
			let spawned = self.spawn('RedneckChickenA');
			if (spawned) spawned.angle = self.angle;
			self.lotag = 128;
		}
	}
}
		
class RedneckChickenSpawner2 : RedneckChickenSpawner1
{
	default
	{
		pic "CHICKENCSPAWN";
	}
	
	override void Tick()
	{
		self.lotag--;
		if (self.lotag < 0)
		{
			let spawned = self.spawn('RedneckChickenC');
			if (spawned) spawned.angle = self.angle;
			self.lotag = 256;
		}
	}
}

class RedneckFeatherSpawner : RedneckChickenSpawner1
{
	default
	{
		pic "FEATHERSPAWN";
	}
	
	override void Tick()
	{
		self.lotag--;
		if (self.lotag < 0)
		{
			self.lotsofstuff('RedneckFeather', random(4, 7));
			self.lotag = 84;
		}
	}
}

class RedneckChickenHeadSpawner : RedneckChickenSpawner1
{
	default
	{
		pic "CHICKENHEADSPAWN";
	}
	
	override void Tick()
	{
		self.lotag--;
		if (self.lotag < 0)
		{
			let spawned = self.spawn('RedneckChickenHead');
			self.lotag = 96;
			if (spawned && !isRRRA()) self.PlayActorSound("POOLBUD");
		}
	}
}

class RedneckChickenLoafSpawner : RedneckChickenSpawner1
{
	default
	{
		pic "LOAFSPAWN";
	}
	
	override void Tick()
	{
		self.lotag--;
		if (self.lotag < 0)
		{
			let spawned = self.spawn('RedneckChickenLoaf');
			if (spawned) spawned.angle = self.angle;
			self.lotag = 448;
		}
	}
}

class RedneckChickenNuggetSpawner : RedneckChickenSpawner1
{
	default
	{
		pic "NUGGETSPAWN";
	}
	
	override void Tick()
	{
		self.lotag--;
		if (self.lotag < 0)
		{
			let spawned = self.spawn('RedneckChickenNugget');
			if (spawned) spawned.angle = self.angle;
			self.lotag = 64;
		}
	}
}

class RedneckChickenRoastSpawner : RedneckChickenSpawner1
{
	default
	{
		pic "ROASTSPAWN";
	}
	
	override void Tick()
	{
		self.lotag--;
		if (self.lotag < 0)
		{
			let spawned = self.spawn('RedneckRoastedChicken');
			if (spawned) spawned.angle = self.angle;
			self.lotag = 512;
		}
	}
}

class RedneckBonelessSpawner : RedneckChickenSpawner1
{
	default
	{
		pic "BONELESSSPAWN";
	}
	
	override void Tick()
	{
		self.lotag--;
		if (self.lotag < 0)
		{
			let spawned = self.spawn('RedneckBonelessChicken');
			if (spawned) spawned.angle = self.angle;
			self.lotag = 224;
		}
	}
}

class RedneckJibsSpawner : RedneckChickenSpawner1
{
	default
	{
		pic "JIBSSPAWN";
	}
	
	override void Tick()
	{
		self.lotag--;
		if (self.lotag < 0)
		{
			self.spawnguts('DukeJibs1', 1);
			self.spawnguts('DukeJibs2', 1);
			self.spawnguts('DukeJibs3', 1);
			self.spawnguts('DukeJibs4', 1);
			self.lotag = 256;
		}
	}
}

//----------------------------------

class RedneckChickenA : DukeActor
{
	default
	{
		spriteset "CHICKENA", "CHICKENB", "CHICKENC";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat = CSTAT_SPRITE_BLOCK_ALL;
		self.vel.X = 2;
		self.clipdist = 2;
		self.Scale = (0.5, 0.40625);
		self.ChangeStat(STAT_ACTOR);
	}
	
	override void Tick()
	{
		if (!ud.chickenplant)
		{
			self.Destroy();
			return;
		}
		let sectp = self.sector;
		if (sectp.lotag == 903)
			self.makeitfall();
			
		int collision = self.movesprite((self.angle.ToVector() * self.vel.X, self.vel.Z), CLIPMASK0);
		switch (sectp.lotag)
		{
			case 901:
				self.SetSpritesetImage(1);
				break;
			case 902:
				self.SetSpritesetImage(2);
				break;
			case 903:
				if (self.pos.Z >= sectp.floorz - 8)
				{
					self.Destroy();
					return;
				}
				break;
			case 904:
				self.Destroy();
				break;
		}
		if (collision > kHitSector)
		{
			self.Destroy();
		}
	}
}

class RedneckChickenB : RedneckChickenA
{
	default
	{
		spritesetindex 1;
	}
}

class RedneckChickenC : RedneckChickenA
{
	default
	{
		spritesetindex 2;
	}
}

//----------------------------------

class RedneckChickenLoaf : DukeActor
{
	default
	{
		pic "CHICKENLOAF";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.cstat = CSTAT_SPRITE_BLOCK_ALL;
		self.vel.X = 2;
		self.Scale = (0.1875, 0.15625);
		self.clipdist = 2;
		self.ChangeStat(STAT_ACTOR);
	}
	
	override void Tick()
	{
		if (!ud.chickenplant)
		{
			self.Destroy();
			return;
		}
		self.makeitfall();
		let collision = self.movesprite((self.angle.ToVector() * self.vel.X, self.vel.Z), CLIPMASK0);
		if (collision > kHitSector)
		{
			self.Destroy();
			return;
		}
		let sectp = self.sector;
		if (sectp.lotag == 903)
		{
			if (self.pos.Z >= sectp.floorz - 4)
			{
				self.Destroy();
				return;
			}
		}
		else if (sectp.lotag == 904)
		{
			self.Destroy();
			return;
		}
	}
}

class RedneckRoastedChicken : RedneckChickenLoaf
{
	default
	{
		pic "ROASTEDCHICKEN";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.vel.X = 1;
		self.Scale = (0.203125, 0.203125);
		self.clipdist = 2;
		self.ChangeStat(STAT_ACTOR);
	}
}

class RedneckChickenNugget : RedneckChickenLoaf
{
	default
	{
		pic "CHICKENNUGGET";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.vel.X = 1;
		self.Scale = (0.125, 0.09375);
		self.clipdist = 0.5;
		self.ChangeStat(STAT_ACTOR);
	}
}

class RedneckBonelessChicken : RedneckChickenLoaf
{
	default
	{
		pic "BONELESSCHICKEN";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.vel.X = 2;
		self.Scale = (0.265625, 0.1875);
		self.clipdist = 2;
		self.ChangeStat(STAT_ACTOR);
	}
}

class RedneckChickenHead : DukeActor
{
	default
	{
		pic "CHICKENHEAD";
	}
	
	override void Initialize(DukeActor spawner)
	{
		self.vel.X = 0;
		self.Scale = (0.203125, 0.15625);
		self.clipdist = 2;
		self.ChangeStat(STAT_ACTOR);
	}
	
	override void Tick()
	{
		if (!ud.chickenplant)
		{
			self.Destroy();
			return;
		}
		self.makeitfall();
		self.movesprite((self.angle.ToVector() * self.vel.X, self.vel.Z), CLIPMASK0);
		if (self.pos.Z >= self.sector.floorz - 8)
		{
			if (self.sector.lotag == 1)
			{
				let j = self.spawn('DukeWaterSplash');
				if (j) j.pos.Z = j.sector.floorz;
			}
			self.Destroy();
		}
	}
}

class RedneckChickenplantButton : DukeActor
{
	default
	{
		spriteset "CHICKENPLANTBUTTON", "CHICKENPLANTBUTTONON";
	}

	override void Initialize(DukeActor spawner)
	{
		ud.chickenplant = 1;
	}

	override bool TriggerSwitch(DukePlayer activator)
	{
		ud.chickenplant = self.spritesetindex;
		self.setSpriteSetImage(1 - self.spritesetindex);
		self.PlayActorSound("SWITCH_ON");
		return true;
	}
}