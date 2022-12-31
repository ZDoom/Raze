extend class DukeActor
{
	
	void spawnguts(class<DukeJibs1> gtype, int n)
	{
		double scale = gs.gutsscale;
		int pal;

		if (self.badguy() && self.scale.X < 0.25)
			scale *= 0.25;

		double gutz = self.pos.Z - 8;
		double c;
		double floorz;
		[c, floorz] = self.sector.getslopes(self.pos.XY);

		if (gutz > floorz - 8)
			gutz = floorz - 8;

		gutz += self.gutsoffset;

		if (self.badguy() && self.pal == 6)
			pal = 6;
		else if (!self.bTRANSFERPALTOJIBS)
			pal = 0;
		else
			pal = self.pal;

		for (int j = 0; j < n; j++)
		{
			// RANDCORRECT version from RR.
			double a = frandom(0, 360);
			double zvel = -2 - frandom(0, 8);
			double vel = frandom(3, 5);
			Vector3 offs;
			offs.Z = gutz - frandom(0, 16);
			offs.Y = frandom(0, 16) - 8;
			offs.X = frandom(0, 16) - 8;

			let spawned = dlevel.SpawnActor(self.sector, offs + self.pos.XY, gtype, -32, (scale, scale), a, vel, zvel, self, STAT_MISC);
			if (spawned && pal != 0)
				spawned.pal = pal;
		}
	}
}


class DukeJibs1 : DukeActor
{
	default
	{
		DukeJibs1.Behavior 0;
		statnum STAT_MISC;
		spriteset "JIBS1", "JIBS6", "JIBS6A", "JIBS6B", "JIBS6C", "JIBS6D", "JIBS6E", "JIBS6F", "JIBS6G";
	}
	
	meta int behavior;
	property behavior: behavior;
	

	override void Tick()
	{
		if(self.vel.X > 0) self.vel.X -= 1/16.;
		else self.vel.X = 0;

		if (!Raze.IsRR())
		{
			if (self.temp_data[0] < 30 * 10)
				self.temp_data[0]++;
			else
			{
				self.Destroy();
				return;
			}
		}

		let sectp = self.sector;

		// this was after the slope calls, but we should avoid calling that for invalid sectors.
		if (sectp == null)
		{
			self.Destroy();
			return;
		}

		double fz, cz;

		[cz, fz] = sectp.GetSlopes(self.pos.XY);

		if (cz >= fz)
		{
			self.Destroy();
			return;
		}

		if (self.pos.Z < fz - 2)
		{
			if (self.temp_data[1] < 2) self.temp_data[1]++;
			else if (sectp.lotag != 2)
			{
				self.temp_data[1] = 0;
				if (self.behavior == 1)
				{
					if (self.counter > 6) self.counter = 0;
					else self.counter++;
				}
				else
				{
					if (self.counter > 2)
						self.counter = 0;
					else self.counter++;
				}
			}

			if (self.vel.Z < 24)
			{
				if (sectp.lotag == 2)
				{
					if (self.vel.Z < 4)
						self.vel.Z += 3 / 16.;
					else self.vel.Z = 4;
				}
				else self.vel.Z += ( gs.gravity - 50/256.);
			}

			self.pos += self.angle.ToVector() * self.vel.X;
			self.pos.Z += self.vel.Z;

			if (Raze.IsRR() && self.pos.Z >= self.sector.floorz)
			{
				self.Destroy();
				return;
			}
		}
		else
		{
			if (self.behavior == 2) // cactus debris only
			{
				self.Destroy();
				return;
			}
			if (self.temp_data[2] == 0)
			{
				if ((self.sector.floorstat & CSTAT_SECTOR_SLOPE))
				{
					self.Destroy();
					return;
				}
				self.temp_data[2]++;
			}

			self.pos.Z = fz - 2;
			self.vel.X = 0;

			if (self.spritesetindex == 1)
			{
				self.temp_data[1]++;
				if ((self.temp_data[1] & 3) == 0 && self.counter < 7)
					self.counter++;
				if (self.temp_data[1] > 20)
				{
					self.Destroy();
					return;
				}
			}
			else if (self.getspritesetsize() > 1)
			{ 
				self.setSpritesetImage(1);
				self.counter = 0; 
				self.temp_data[1] = 0; 
			}
			else
			{
				self.Destroy();
				return;
			}
		}
		if (Raze.IsRR() && self.sector.lotag == 800 &&  self.pos.Z >= self.sector.floorz - 8)
		{
			self.Destroy();
			return;
		}
		// always keep the sector up to date. The orignal code did not.
		self.SetPosition(self.pos);
	}
	
	override bool animate(tspritetype tspr) 
	{ 
		if (Raze.isRRRA() && tspr.pal == 19)
			tspr.shade = -127;
	
		if (spritesetindex == 1)
		{
			tspr.SetSpritePic(self, 1 + self.counter);
		}
		if (tspr.pal == 6) tspr.shade = -120;

		if (self.sector.shadedsector == 1)
			tspr.shade = 16;
		
		return true;
	}
	
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeJibs2 : DukeJibs1
{
	default
	{
		pic "JIBS2";
	}
	
	override void Initialize()
	{
		self.scale *= 0.25; // only Duke needs this.
	}
}

class RedneckJibs2 : DukeJibs1
{
	default
	{
		pic "JIBS2";
	}
}

class DukeJibs3 : DukeJibs1
{
	default
	{
		pic "JIBS3";
	}
}

class DukeJibs4 : DukeJibs1
{
	default
	{
		pic "JIBS4";
	}
}

class DukeJibs5 : DukeJibs1
{
	default
	{
		pic "JIBS5";
	}
}

class DukeJibs6 : DukeJibs1
{
	default
	{
		spritesetindex 1;
	}
	
	override void Initialize()
	{
		if (Raze.isRR()) self.scale *= 0.5; // only RR needs this.
		self.setSpriteSetImage(1);
	}
}

class DukeHeadJib1 : DukeJibs1
{
	default
	{
		pic "HEADJIB1";
	}
}

class DukeArmJib1 : DukeJibs1
{
	default
	{
		pic "ARMJIB1";
	}
}

class DukeLegJib1 : DukeJibs1
{
	default
	{
		pic "LEGJIB1";
	}
}

class DukeLizmanHead : DukeJibs1
{
	default
	{
		pic "LIZMANHEAD1";
	}
}

class DukeLizmanArm : DukeJibs1
{
	default
	{
		pic "LIZMANARM1";
	}
}

class DukeLizmanLeg : DukeJibs1
{
	default
	{
		pic "LIZMANLEG1";
	}
}

class DukePlayerTorso : DukeJibs1
{
	default
	{
		pic "DUKETORSO";
		DukeJibs1.Behavior 1;
	}
}

class DukePlayerGun : DukePlayerTorso
{
	default
	{
		pic "DUKEGUN";
	}
}

class DukePlayerLeg : DukePlayerTorso
{
	default
	{
		pic "DUKELEG";
	}
}

// --------------------------- RR Jibs

class RedneckBillyJibA : DukeJibs1
{
	default
	{
		pic "BILLYJIBA";
	}
}

class RedneckBillyJibB : DukeJibs1
{
	default
	{
		pic "BILLYJIBB";
	}
}

class RedneckHulkJibA : DukeJibs1
{
	default
	{
		pic "HULKJIBA";
	}
}

class RedneckHulkJibB : DukeJibs1
{
	default
	{
		pic "HULKJIBB";
	}
}

class RedneckHulkJibC : DukeJibs1
{
	default
	{
		pic "HULKJIBC";
	}
}

class RedneckMinJibA : DukeJibs1
{
	default
	{
		pic "MINJIBA";
	}
}

class RedneckMinJibB : DukeJibs1
{
	default
	{
		pic "MINJIBB";
	}
}

class RedneckMinJibC : DukeJibs1
{
	default
	{
		pic "MINJIBC";
	}
}

class RedneckCootJibA : DukeJibs1
{
	default
	{
		pic "COOTJIBA";
	}
}

class RedneckCootJibB : DukeJibs1
{
	default
	{
		pic "COOTJIBB";
	}
}

class RedneckCootJibC : DukeJibs1
{
	default
	{
		pic "COOTJIBC";
	}
}

// --------------------------- RRRA Jibs

class RedneckBikeJibA : DukeJibs1
{
	default
	{
		pic "BIKEJIBA";
	}
}

class RedneckBikeJibB : DukeJibs1
{
	default
	{
		pic "BIKEJIBB";
	}
}

class RedneckBikeJibC : DukeJibs1
{
	default
	{
		pic "BIKEJIBC";
	}
}

class RedneckBikerJibA : DukeJibs1
{
	default
	{
		pic "BIKERJIBA";
	}
}

class RedneckBikerJibB : DukeJibs1
{
	default
	{
		pic "BIKERJIBB";
	}
}

class RedneckBikerJibC : DukeJibs1
{
	default
	{
		pic "BIKERJIBC";
	}
}

class RedneckBikerJibD : DukeJibs1
{
	default
	{
		pic "BIKERJIBd";
	}
}

class RedneckCheerJibA : DukeJibs1
{
	default
	{
		pic "CHEERJIBA";
	}
}

class RedneckCheerJibB : DukeJibs1
{
	default
	{
		pic "CHEERJIBB";
	}
}

class RedneckCheerJibC : DukeJibs1
{
	default
	{
		pic "CHEERJIBC";
	}
}

class RedneckCheerJibD : DukeJibs1
{
	default
	{
		pic "CHEERJIBd";
	}
}

class RedneckFBoatJibA : DukeJibs1
{
	default
	{
		pic "FBOATJIBA";
	}
}

class RedneckFBoatJibB : DukeJibs1
{
	default
	{
		pic "FBOATJIBB";
	}
}

class RedneckMamaJibA : DukeJibs1
{
	default
	{
		pic "MAMAJIBA";
	}
}

class RedneckMamaJibB : DukeJibs1
{
	default
	{
		pic "MAMAJIBB";
	}
}

class RedneckRabbitJibA : DukeJibs1
{
	default
	{
		pic "RABBITJIBA";
	}
	
	override void Initialize()
	{
		self.Scale = (0.28125, 0.28125);
	}
}

class RedneckRabbitJibB : DukeJibs1
{
	default
	{
		pic "RABBITJIBB";
	}
	
	override void Initialize()
	{
		self.Scale = (0.5625, 0.5625);
	}
}

class RedneckRabbitJibC : DukeJibs1
{
	default
	{
		pic "RABBITJIBC";
	}
	
	override void Initialize()
	{
		self.Scale = (0.84375, 0.84375);
	}
}


// ---------------------------------

class RedneckCactusDebris1 : DukeJibs1
{
	default
	{
		pic "CACTUSDEBRIS1";
		DukeJibs1.Behavior 2;
	}
}

class RedneckCactusDebris2 : RedneckCactusDebris1
{
	default
	{
		pic "CACTUSDEBRIS2";
	}
}

