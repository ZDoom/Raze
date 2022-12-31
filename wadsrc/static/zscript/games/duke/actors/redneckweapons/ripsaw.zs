
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class RedneckSawBlade : DukeProjectile
{
	default
	{
		+CANHURTSHOOTER;
		spriteset "SAWBLADE", "SAWBLADE2", "SAWBLADE3",  "SAWBLADE4",  "SAWBLADE5",  "SAWBLADE6",  "SAWBLADE7",  "SAWBLADE8",
			"CHEERBLADE", "CHEERBLADE2", "CHEERBLADE3", "CHEERBLADE4";
		Strength THROWSAW_WEAPON_STRENGTH;
	}

	override bool weaponhitwall(walltype wal)
	{
		if (Raze.tileflags(wal.walltexture) & Duke.TFLAG_NOCIRCLEREFLECT)
		{
			self.Destroy();
			return true;
		}
		if (self.extra <= 0)
		{
			self.pos += self.angle.ToVector() * 8;
			let Owner = self.ownerActor;
			if (!Owner || !(Owner.bALTPROJECTILESPRITE)) // depends on the shooter. Urgh...
			{
				let j = self.spawn("RedneckCircleStuck");
				if (j)
				{
					j.scale = (0.125, 0.125);
					j.cstat = CSTAT_SPRITE_ALIGNMENT_WALL;
					j.angle += 90;
					j.clipdist = self.scale.X * self.spriteWidth() * 0.125;
				}
			}
			self.Destroy();
			return true;
		}
		if (!dlevel.isMirror(wal))
		{
			self.extra -= 20;
			self.yint--;
		}

		let k = wal.delta().Angle();
		self.angle = k * 2 - self.angle;
		return true;
	}
	
	override bool animate(tspritetype tspr)
	{
		int frame;
		if (!OwnerActor || !(OwnerActor.bALTPROJECTILESPRITE)) frame = ((PlayClock >> 4) & 7);
		else frame = 8 + ((PlayClock >> 4) & 3);
		tspr.SetSpritePic(self, frame);
		return true;
	}
}


class RedneckCircleStuck : DukeActor
{
	default
	{
		pic "CIRCLESTUCK";
		Strength 0;
	}

}
