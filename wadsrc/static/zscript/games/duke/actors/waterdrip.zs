class DukeWaterDrip : DukeActor
{
	default
	{
		ScaleX 0.375;
		ScaleY 0.375;
		statnum STAT_STANDABLE;
		pic "WATERDRIP";
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner && (owner.statnum == STAT_PLAYER || owner.statnum == STAT_ACTOR))
		{
			self.shade = 32;
			if (owner.pal != 1)
			{
				self.pal = 2;
				self.pos.Z -= 18;
			}
			else self.pos.Z -= 13;
			self.angle = (Duke.GetViewPlayer().actor.pos.XY - self.pos.XY).Angle();
			self.vel.X = frandom(1, 3);
			self.DoMove(CLIPMASK0);
		}
		else if (owner == self)
		{
			self.pos.Z += 4;
			self.temp_pos.Z = self.pos.Z;
			self.temp_data[1] = random(0, 127);
		}
	}
	
	override void Tick()
	{
		if (self.temp_data[1])
		{
			self.temp_data[1]--;
			if (self.temp_data[1] == 0)
				self.cstat &= ~CSTAT_SPRITE_INVISIBLE;
		}
		else
		{
			self.makeitfall();
			self.DoMove(CLIPMASK0);
			if(self.vel.X > 0) self.vel.X -= 1/8.;

			if (self.vel.Z == 0)
			{
				self.cstat |= CSTAT_SPRITE_INVISIBLE;

				if (self.pal != 2 && (self.hitag == 0 || Raze.isRR()))
					self.PlayActorSound(DukeSnd.SOMETHING_DRIPPING);

				if (self.ownerActor != self)
				{
					self.Destroy();
				}
				else
				{
					self.pos.Z = self.temp_pos.Z;
					self.backuppos();
					self.temp_data[1] = random(48, 79);
				}
			}
		}
	}

	
}



