class DukeFrameEffect : DukeActor
{
	default
	{
		Pic "SMALLSMOKE";
	}
	
	
	override void Initialize()
	{
		if (!self.mapSpawned && self.ownerActor)
		{
			self.scale = self.ownerActor.scale;
		}
		else
		{
			self.Scale = (0, 0);
		}
		self.ChangeStat(STAT_MISC);
	}
	
	override void Tick()
	{
		let Owner = self.ownerActor;
		if (Owner)
		{
			self.temp_data[0]++;

			if (self.temp_data[0] > 7)
			{
				self.Destroy();
				return;
			}
			else if (self.temp_data[0] > 4) self.cstat |= CSTAT_SPRITE_TRANS_FLIP | CSTAT_SPRITE_TRANSLUCENT;
			else if (self.temp_data[0] > 2) self.cstat |= CSTAT_SPRITE_TRANSLUCENT;
			self.xoffset = Owner.xoffset;
			self.yoffset = Owner.yoffset;
		}
	}
	
	override bool animate(tspritetype t)
	{
		let OwnerAc = self.ownerActor;
		if (OwnerAc)
		{
			if (OwnerAc.isPlayer())
				if (ud.cameraactor == nullptr)
					if (Duke.GetViewPlayer() == OwnerAc.GetPlayer() && display_mirror == 0)
					{
						t.ownerActor = nullptr;
						t.scale = (0, 0);
						return true;
					}
			if ((OwnerAc.cstat & CSTAT_SPRITE_INVISIBLE) == 0)
			{
				if (!OwnerAc.isPlayer() || !Raze.isRR()) t.SetSpritePic(OwnerAc, -1);
				else t.SetSpritePic(OwnerAc, 0);
				t.pal = OwnerAc.pal;
				t.shade = OwnerAc.shade;
				t.angle = OwnerAc.angle;
				t.cstat = CSTAT_SPRITE_TRANSLUCENT | OwnerAc.cstat;
			}
		}
		return true;
	}
	
}

