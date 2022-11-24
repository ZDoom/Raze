
class RedneckChickenSpawner1 : DukeActor
{
	default
	{
		scaleX 0;
		scaleY 0;
		clipdist 0;
		lotag 0;
		statnum STAT_CHICKENPLANT;
		pic "CHICKENASPAWN";
	}
	
	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_INVISIBLE;
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
			if (spawned && !Raze.isRRRA()) self.PlayActorSound("POOLBUD");
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
			let spawned = self.spawn('RedneckChickenRoast');
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
		clipdist 2;
		scaleX 0.5;
		scaleY 0.40625;
		statnum STAT_ACTOR;
		spriteset "CHICKENA", "CHICKENB", "CHICKENC";
	}
	
	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_BLOCK_ALL;
		self.vel.X = 2;
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
		statnum STAT_ACTOR;
		clipdist 2;
		scaleX 0.1875;
		scaleY 0.15625;
		pic "CHICKENLOAF";
	}
	
	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_BLOCK_ALL;
		self.vel.X = 2;
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
		scaleX 0.203125;
		scaleY 0.203125;
		clipdist 2;
		pic "ROASTEDCHICKEN";
	}
	
	override void Initialize()
	{
		self.vel.X = 1;
	}
}

class RedneckChickenNugget : RedneckChickenLoaf
{
	default
	{
		scaleX 0.125;
		scaleY 0.09375;
		clipdist 0.5;
		pic "CHICKENNUGGET";
	}
	
	override void Initialize()
	{
		self.vel.X = 1;
	}
}

class RedneckBonelessChicken : RedneckChickenLoaf
{
	default
	{
		scaleX 0.265625;
		scaleY 0.1875;
		clipdist 2;
		pic "BONELESSCHICKEN";
	}
	
	override void Initialize()
	{
		self.vel.X = 2;
	}
}

class RedneckChickenHead : DukeActor
{
	default
	{
		statnum STAT_ACTOR;
		scaleX 0.203125;
		scaleY 0.15625;
		clipdist 2;
		pic "CHICKENHEAD";
	}
	
	override void Initialize()
	{
		self.vel.X = 0;
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
				let j = self.spawn('DukeWaterSplash2');
				if (j) j.pos.Z = j.sector.floorz;
			}
			self.Destroy();
		}
	}
}

