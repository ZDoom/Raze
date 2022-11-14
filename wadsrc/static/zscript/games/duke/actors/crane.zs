
class DukeCrane : DukeActor
{
	default
	{
		spriteset "CRANE", "CRANE1", "CRANE2";
		statnum STAT_STANDABLE;
	}
	
	enum EPic
	{
		PIC_DEFAULT = 0,
		PIC_OPEN = 1,
		PIC_CLOSED = 2,
	}

	Vector3 cranepos;
	Vector2 polepos;
	DukeActor poleactor;
	bool isactive;

	const CRANE_STEP = 16.;

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void Initialize()
	{
		let sect = self.sector;
		self.cstat |= CSTAT_SPRITE_BLOCK_ALL | CSTAT_SPRITE_ONE_SIDE;

		self.setSpritePic(PIC_CLOSED);
		self.pos.Z = sect.ceilingz + 48;

		self.cranepos = self.pos;
		self.poleactor = null;

		DukeStatIterator it;
		for (DukeActor poleactor = it.first(DukeActor.STAT_DEFAULT); poleactor; poleactor = it.next())
		{
			if (self.hitag == poleactor.hitag && poleactor is "DukeCranePole")
			{
				self.poleactor = poleactor;
				self.temp_sect = poleactor.sector;

				poleactor.scale = (0.75, 2);
				self.polepos = poleactor.pos.XY;

				poleactor.shade = self.shade;
				poleactor.SetPosition(self.pos);
				break;
			}
		}

		self.ownerActor = null;
		self.extra = 8;
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	override void Tick()
	{
		let sectp = self.sector;
		double xx;

		//self.temp_data[0] = state
		//self.temp_data[1] = checking sector number

		if (self.vel.X != 0) self.getGlobalZ();

		if (self.temp_data[0] == 0) //Waiting to check the sector
		{
			DukeSectIterator it;
			for (DukeActor a2 = it.First(self.temp_sect); a2; a2 = it.Next())
			{
				switch (a2.statnum)
				{
				case STAT_ACTOR:
				case STAT_ZOMBIEACTOR:
				case STAT_STANDABLE:
				case STAT_PLAYER:
					self.angle = (self.polepos - self.pos.XY).Angle();
					a2.SetPosition(( self.polepos, a2.pos.Z ));
					self.temp_data[0]++;
					return;
				}
			}
		}

		else if (self.temp_data[0] == 1)
		{
			if (self.vel.X < 11.5)
			{
				self.setSpritePic(PIC_OPEN);
				self.vel.X += 0.5;
			}
			self.DoMove(CLIPMASK0);
			if (self.sector == self.temp_sect)
				self.temp_data[0]++;
		}
		else if (self.temp_data[0] == 2 || self.temp_data[0] == 7)
		{
			self.pos.Z += 6;

			if (self.temp_data[0] == 2)
			{
				if ((sectp.floorz - self.pos.Z) < 64)
					if (self.spritesetindex != PIC_DEFAULT) self.setSpritePic(self.spritesetindex - 1);

				if ((sectp.floorz - self.pos.Z) < 20)
					self.temp_data[0]++;
			}
			if (self.temp_data[0] == 7)
			{
				if ((sectp.floorz - self.pos.Z) < 64)
				{
					if (self.spritesetindex != PIC_DEFAULT) self.setSpritePic(self.spritesetindex - 1);
					else
					{
						if (self.isactive)
						{
							let pp = findplayer();
							// fixme: Sounds really need to be better abstracted...
							pp.actor.PlayActorSound(Raze.isRR() ? RRSnd.YEHAA16 : DukeSnd.DUKE_GRUNT);
							if (pp.on_crane == self)
								pp.on_crane = null;
						}
						self.temp_data[0]++;
						self.isactive = false;
						self.ownerActor = null;
					}
				}
			}
		}
		else if (self.temp_data[0] == 3)
		{
			self.setSpritePic(self.spritesetindex + 1);
			if (self.spritesetindex == PIC_CLOSED)
			{
				let plr = Duke.checkcursectnums(self.temp_sect);
				if (plr != null && plr.on_ground)
				{
					self.isactive = true;
					self.ownerActor = null;
					plr.on_crane = self;
					// fixme: Sounds really need to be better abstracted...
					plr.actor.PlayActorSound(Raze.isRR() ? RRSnd.YEHAA16 : DukeSnd.DUKE_GRUNT);
					plr.settargetangle(self.angle + 180);
				}
				else
				{
					DukeSectIterator it;
					for (DukeActor a2 = it.First(self.temp_sect); a2; a2 = it.Next())
					{
						switch (a2.statnum)
						{
						case 1:
						case 6:
							self.OwnerActor = a2;
							break;
						}
					}
				}

				self.temp_data[0]++;//Grabbed the sprite
				self.temp_data[2] = 0;
				return;
			}
		}
		else if (self.temp_data[0] == 4) //Delay before going up
		{
			self.temp_data[2]++;
			if (self.temp_data[2] > 10)
				self.temp_data[0]++;
		}
		else if (self.temp_data[0] == 5 || self.temp_data[0] == 8)
		{
			if (self.temp_data[0] == 8 && self.spritesetindex < PIC_CLOSED)
				if ((sectp.floorz - self.pos.Z) > 32)
					self.setSpritePic(self.spritesetindex + 1);

			if (self.pos.Z < self.cranepos.Z)
			{
				self.temp_data[0]++;
				self.vel.X = 0;
			}
			else
				self.pos.Z -= 6;
		}
		else if (self.temp_data[0] == 6)
		{
			if (self.vel.X < 12)
				self.vel.X += 0.5;
			self.angle = (self.cranepos.XY - self.pos.XY).Angle();
			self.DoMove(CLIPMASK0);
			if ((self.pos.XY - self.cranepos.XY).LengthSquared() < 8 * 8)
				self.temp_data[0]++;
		}

		else if (self.temp_data[0] == 9)
			self.temp_data[0] = 0;

		if (self.poleactor)
			self.poleactor.SetPosition(self.pos.plusZ(-34));

		let Owner = self.OwnerActor;
		if (Owner != null || self.isactive)
		{
			let p = self.findplayer();

			int j = self.ifhitbyweapon();
			if (j >= 0)
			{
				if (self.isactive && p.on_crane == self)
					p.on_crane = null;
				self.isactive = false;
				self.ownerActor = null;
				self.setSpritePic(PIC_DEFAULT);
				return;
			}

			if (Owner != null)
			{
				Owner.SetPosition(self.pos);
				Owner.backuppos();
				self.vel.Z = 0;
			}
			else if (self.isactive)
			{
				let ang = p.angle();
				p.backupxyz();
				p.PlayerNowPosition.XY = self.pos.XY - CRANE_STEP * ang.ToVector();
				p.PlayerNowPosition.Z = self.pos.Z + 2;
				p.actor.SetPosition(p.PlayerNowPosition);
				p.cursector = p.actor.sector;
			}
		}
	}
}

class DukeCranePole : DukeActor
{
	default
	{
		pic "CRANEPOLE";
	}
}
