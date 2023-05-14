class DukeForceSphere : DukeActor
{
	default
	{
		pic "FORCESPHERE";
		+NOFLOORPAL;
	}
	
	
	override void Initialize(DukeActor spawner)
	{
		if (self.mapSpawned)
		{
			self.cstat = CSTAT_SPRITE_INVISIBLE;
			self.ChangeStat(STAT_ZOMBIEACTOR);
		}
		else
		{
			self.scale = (1 / 64., 1 / 64.);
			self.ChangeStat(STAT_MISC);
		}
	}
	
	void TickParent()
	{
		let sectp = self.sector;
		if (self.yint == 0)
		{
			self.yint = 1;

			for (double l = 90; l < 270; l += 22.5)
				for (double j = 0; j < 360; j += 22.5)
				{
					let k = self.spawn("DukeForceSphere");
					if (k)
					{
						k.cstat = CSTAT_SPRITE_BLOCK_ALL | CSTAT_SPRITE_YCENTER;
						k.clipdist = 16;
						k.angle = j;
						k.vel.Z = sin(l) * 2;
						k.vel.X = cos(l) * 2;
						k.ownerActor = self;
					}
				}
		}

		if (self.temp_data[3] > 0)
		{
			if (self.vel.Z < 24)
				self.vel.Z += 0.75;
			self.pos.Z += self.vel.Z;
			if (self.pos.Z > sectp.floorz)
				self.pos.Z = sectp.floorz;
			self.temp_data[3]--;
			if (self.temp_data[3] == 0)
			{
				self.Destroy();
				return;
			}
			else if (self.temp_data[2] > 10)
			{
				DukeStatIterator it;
				for (let aa = it.First(STAT_MISC); aa; aa = it.Next())
				{
					if (aa.ownerActor == self && aa is 'DukeForceSphere')
						aa.temp_data[1] = random(1, 64);
				}
				self.temp_data[3] = 64;
			}
		}
	}
	
	void TickChild()
	{
		double size = self.scale.X;
		if (self.temp_data[1] > 0)
		{
			self.temp_data[1]--;
			if (self.temp_data[1] == 0)
			{
				self.Destroy();
				return;
			}
		}
		let Owner = self.ownerActor;
		if (!Owner) return;
		if (Owner.temp_data[1] == 0)
		{
			if (self.counter < 64)
			{
				self.counter++;
				size += 0.046875;
			}
		}
		else
			if (self.counter > 64)
			{
				self.counter--;
				size -= 0.046875;
			}

		self.pos = Owner.pos;;
		self.angle += Raze.BAngToDegree * Owner.counter;

		size = clamp(size, 1 / 64., 1.);

		self.scale = (size, size);
		self.shade = ((size * 32) - 48);

		for (int j = self.counter; j > 0; j--)
			self.DoMove(CLIPMASK0);
	}
	
	override void Tick()
	{
		if (self.statnum == STAT_MISC) TickChild();
		else TickParent();
	}
	
	override bool Animate(tspritetype t)
	{
		let OwnerAc = self.ownerActor;
		if (self.statnum == STAT_MISC && OwnerAc)
		{
			let plr = Duke.GetViewPlayer().actor;
			let sqa = (OwnerAc.pos.XY - plr.pos.XY).Angle();
			let sqb = (OwnerAc.pos.XY - t.pos.XY).Angle();

			if (absangle(sqa, sqb) > 90)
			{
				double dist1 = (OwnerAc.pos.XY - t.pos.XY).LengthSquared();
				double dist2 = (OwnerAc.pos.XY - plr.pos.XY).LengthSquared();
				if (dist1 < dist2)
					t.scale = (0, 0);
			}
		}
		return true;
	}
	
	override void onHit(DukeActor hitter)
	{
		self.scale.X = (0);
		let Owner = self.ownerActor;
		if (Owner)
		{
			Owner.counter = 32;
			Owner.temp_data[1] = !Owner.temp_data[1];
			Owner.temp_data[2] ++;
		}
		self.spawn("DukeExplosion2");
	}
}
