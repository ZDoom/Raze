class DukeViewscreen : DukeActor
{
	default
	{
		spriteset "VIEWSCREEN", "STATIC", "VIEWSCR";
		+NOFALLER;
		+NOFLOORPAL;
	}

	const VIEWSCR_DIST = 1024;	// was originally 2048, was increased to 8192 by EDuke32 and RedNukem, but with high resolutions the resulting 512 map units are still too low.
	
	override void Initialize(DukeActor spawner)
	{
		self.ownerActor = self;
		self.lotag = 1;
		self.extra = 1;
		self.ChangeStat(STAT_STANDABLE);
	}
	
	override void Tick()
	{
		if (self.scale.X == 0) self.Destroy();
		else
		{
			let p = self.findplayer();

			double x = (self.pos - p.actor.pos).LengthSquared(); // the result from findplayer is not really useful.
			if (x >= VIEWSCR_DIST * VIEWSCR_DIST && camsprite == self)
			{
				camsprite = null;
				self.yint = 0;
				self.counter = 0;
			}
		}
	}

	override bool onUse(DukePlayer user)
	{
		DukeStatIterator it;
		for(let acti = it.First(STAT_ACTOR); acti; acti = it.Next())
		{
			if (acti.bCAMERA && acti.yint == 0 && self.hitag == acti.lotag)
			{
				acti.yint = 1; //Using this camera
				if (user == Duke.GetViewPlayer()) Duke.PlaySound("MONITOR_ACTIVE");

				self.ownerActor = acti;
				self.yint = 1;
				camsprite = self;

				user.newOwner = acti;
				Raze.forceSyncInput(Duke.GetPlayerIndex(user));
				return true;
			}
		}
		user.clearcameras();
		return true;
	}
	
	override bool animate(tspritetype tspr)
	{
		let actor = DukeActor(tspr.ownerActor);
		let hitowner = actor.hitOwnerActor;
		if (camsprite != null && hitowner && hitowner.counter == 1)
		{
			tspr.SetSpritePic(self, 1);
			tspr.cstat &= ~ (CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP);
			tspr.cstat |= randomFlip();
			tspr.scale.X += (0.125);
			tspr.scale.Y += (0.125);
		}
		else if (camsprite && camsprite == hitowner)
		{
			tspr.SetSpritePic(self, 2);
		}
		return true;
	}
}

class DukeViewscreen2 : DukeViewscreen
{
	default
	{
		spriteset "VIEWSCREEN", "STATIC", "VIEWSCR";
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeCamera : DukeActor
{
	default
	{
		pic "CAMERA1";
		+CAMERA;
		+ALWAYSROTATE1;
	}

	override void Initialize(DukeActor spawner)
	{
		if (gs.camerashitable) self.cstat = CSTAT_SPRITE_BLOCK_ALL;
		else self.cstat = 0;

		if (ud.multimode < 2 && self.pal != 0)
		{
			self.scale = (0, 0);
			self.ChangeStat(STAT_MISC);
			return;
		}
		else self.pal = 0;
		self.extra = 1;
		self.ChangeStat(STAT_ACTOR);
	}
	
	override void Tick()
	{
		if (self.counter == 0)
		{
			self.temp_data[1] += 8;
			if (gs.camerashitable)
			{
				int j = self.ifhitbyweapon();
				if (j >= 0)
				{
					self.counter = 1; // static
					self.cstat = CSTAT_SPRITE_INVISIBLE;
					for (int x = 0; x < 5; x++)
						self.RANDOMSCRAP();
					return;
				}
			}
			
			if (self.hitag > 0)
			{
				let angle = 360. / 256;

				if (self.temp_data[1] < self.hitag)
					self.angle += angle;
				else if (self.temp_data[1] < (self.hitag * 3))
					self.angle -= angle;
				else if (self.temp_data[1] < (self.hitag * 4))
					self.angle += angle;
				else
				{
					self.angle += angle;
					self.temp_data[1] = 0;
				}
			}
		}
	}
}
			