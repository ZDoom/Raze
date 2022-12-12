
class DukeTransporterStar : DukeActor
{
	default
	{
		pic "TRANSPORTERSTAR";
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner == nullptr || owner == self) 
		{
			scale = (0, 0);
			return;
		}
		if (owner.statnum == STAT_PROJECTILE)
		{
			self.scale = (0.125, 0.125);
		}
		else
		{
			self.scale = (0.75, 1);
			if (owner.statnum == STAT_PLAYER || owner.badguy())
				self.pos.Z -= 32;
		}

		self.cstat = CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
		self.angle = owner.angle;

		self.vel.X = 8;
		self.DoMove(CLIPMASK0);
		self.SetPosition(self.pos);
		self.ChangeStat(STAT_MISC);
		self.shade = -127;
	}
}


class DukeTransporterBeam : DukeActor
{
	default
	{
		pic "TRANSPORTERBEAM";
	}
	
	override void Initialize()
	{
		let owner = self.ownerActor;
		if (owner == nullptr || owner == self) 
		{
			scale = (0, 0);
			return;
		}
		self.scale = (0.484375, REPEAT_SCALE);
		self.pos.Z = owner.sector.floorz - gs.playerheight;

		self.cstat = CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
		self.angle = owner.angle;

		self.vel.X = 8;
		self.DoMove(CLIPMASK0);
		self.SetPosition(self.pos);
		self.ChangeStat(STAT_MISC);
		self.shade = -127;
	}
}

