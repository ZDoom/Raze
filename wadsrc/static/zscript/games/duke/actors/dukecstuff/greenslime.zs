class DukeGreenSlime : DukeActor
{
	default
	{
		spriteset "GREENSLIME", "GREENSLIME1", "GREENSLIME2", "GREENSLIME3", "GREENSLIME4", "GREENSLIME5", "GREENSLIME6", "GREENSLIME7";
		+INTERNAL_BADGUY;
		+KILLCOUNT;
		+DONTDIVEALIVE;
		+FORCESECTORSHADE;
		+SHRINKAUTOAIM;
		+NOHITJIBS;
		sparkoffset -3;
	}

	override void Initialize()
	{
		self.scale = (0.625, 0.625);
		self.clipdist = 20;
		self.extra = 1;

	}

	override void PlayFTASound(int mode)
	{
		self.PlayActorSound("SLIM_RECOG");
	}


	override void Tick()
	{
		let sectp = self.sector;
		int j;

		self.temp_data[1] += 128;

		if (sectp.floorstat & CSTAT_SECTOR_SKY)
		{
			self.Destroy();
			return;
		}

		double xx;
		DukePlayer p;
		[p, xx] = self.findplayer();
		let pactor = p.actor;

		if (xx > 1280)
		{
			self.timetosleep++;
			if (self.timetosleep > Duke.SLEEPTIME)
			{
				self.timetosleep = 0;
				self.ChangeStat(STAT_ZOMBIEACTOR);
				return;
			}
		}

		if (self.counter == -5) // FROZEN
		{
			self.temp_data[3]++;
			if (self.temp_data[3] > 280)
			{
				self.pal = 0;
				self.counter = 0;
				return;
			}
			self.makeitfall();
			self.cstat = CSTAT_SPRITE_BLOCK_ALL;
			self.setSpriteSetImage(2);
			self.extra = 1;
			self.pal = 1;
			j = self.ifhitbyweapon();
			if (j >= 0)
			{
				if (j == 1) // freeze damage
					return;
				for (j = 16; j >= 0; j--)
				{
					let a = frandom(0, 360);
					let vel = frandom(0, 2) + 2;
					let zvel = 4 - frandom(0, 4);

					static const name pieces[] = {'DukeGlassPieces', 'DukeGlassPieces1', 'DukeGlassPieces2'};
					let k = dlevel.SpawnActor(self.sector, self.pos, pieces[j % 3], -32, (0.5625, 0.5625), a, vel, zvel, self, STAT_MISC);
					if (k) k.pal = 1;
				}
				self.addkill();
				self.PlayActorSound("GLASS_BREAKING");
				self.Destroy();
			}
			else if (xx < 64 && p.quick_kick == 0)
			{
				let ang = absangle(pactor.angle, (self.pos.XY - pactor.pos.XY).Angle());
				if (ang < 22.5)
					p.quick_kick = 14;
			}

			return;
		}

		if (xx < 99.75)
			self.cstat = 0;
		else self.cstat = CSTAT_SPRITE_BLOCK_ALL;

		if (self.counter == -4) //On the player
		{
			if (pactor.extra < 1)
			{
				self.counter = 0;
				return;
			}

			self.SetPosition(self.pos);

			self.angle = pactor.angle;

			if ((p.PlayerInput(Duke.SB_FIRE) || (p.quick_kick > 0)) && pactor.extra > 0)
				if (p.quick_kick > 0 || (p.curr_weapon != DukeWpn.HANDREMOTE_WEAPON && p.curr_weapon != DukeWpn.HANDBOMB_WEAPON && p.curr_weapon != DukeWpn.TRIPBOMB_WEAPON && p.ammo_amount[p.curr_weapon] >= 0))
				{
					for (int x = 0; x < 8; x++)
					{
						let a = frandom(0, 360);
						let vel = frandom(0, 4) + 4;
						let zvel = -frandom(0, 16) - self.vel.Z * 0.25;

						let spawned = dlevel.SpawnActor(self.sector, self.pos.plusZ(-8), "DukeScrap", -8, (0.75, 0.75), a, vel, zvel, self, STAT_MISC);
						if (spawned)
						{
							if (spawned) spawned.spriteextra = DukeScrap.Scrap3 + random(0, 3);
							spawned.pal = 6;
						}
					}

					self.PlayActorSound("SLIM_DYING");
					self.PlayActorSound("SQUISHED");
					if (random(0, 255) < 32)
					{
						let spawned = self.spawn("DukeBloodPool");
						if (spawned) spawned.pal = 0;
					}
					self.addkill();
					self.counter = -3;
					if (p.somethingonplayer == self)
						p.somethingonplayer = nullptr;
					self.Destroy();
					return;
				}

			self.pos.Z = pactor.pos.Z + pactor.viewzoffset + 8 + p.pyoff - (self.temp_data[2] + tan(pactor.pitch) * 2048.) * zmaptoworld;

			if (self.temp_data[2] > 512)
				self.temp_data[2] -= 128;

			if (self.temp_data[2] < 348)
				self.temp_data[2] += 128;

			if (p.newOwner != nullptr)
			{
				p.newOwner = nullptr;
				pactor.restoreloc();

				p.cursector = Raze.updatesector(pactor.pos.XY, p.cursector);

				DukeStatIterator it;
				for (let ac = it.First(STAT_ACTOR); ac; ac = it.Next())
				{
					if (ac.bCAMERA) ac.yint = 0;
				}
			}

			if (self.temp_data[3] > 0)
			{
				static const int frames[] = { 5,5,6,6,7,7,6,5 };

				self.setSpriteSetImage(frames[self.temp_data[3]]);

				if (self.temp_data[3] == 5)
				{
					let psp = pactor;
					psp.extra -= random(5, 8);
					self.PlayActorSound("SLIM_ATTACK");
				}

				if (self.temp_data[3] < 7) self.temp_data[3]++;
				else self.temp_data[3] = 0;

			}
			else
			{
				self.setSpriteSetImage(5);
				if (Duke.rnd(32))
					self.temp_data[3] = 1;
			}

			double add = (Raze.BobVal(self.temp_data[1]) * 2) * REPEAT_SCALE;
			self.scale = (0.3125 + add, 0.234375 + add);
			self.pos.XY = pactor.pos.XY + pactor.angle.ToVector() * 8;
			return;
		}

		else if (self.vel.X < 4 && xx < 48)
		{
			if (p.somethingonplayer == nullptr)
			{
				p.somethingonplayer = self;
				if (self.counter == 3 || self.counter == 2) //Falling downward
					self.temp_data[2] = (12 << 8);
				else self.temp_data[2] = -(13 << 8); //Climbing up duke
				self.counter = -4;
			}
		}

		j = self.ifhitbyweapon();
		if (j >= 0)
		{
			self.PlayActorSound("SLIM_DYING");

			if (p.somethingonplayer == self)
				p.somethingonplayer = nullptr;

			if (j == 1)	// freeze damage
			{
				self.PlayActorSound("SOMETHINGFROZE"); 
				self.counter = -5; self.temp_data[3] = 0;
				return;
			}
			self.addkill();

			if (random(0, 255) < 32)
			{
				let spawned = self.spawn("DukeBloodPool");
				if (spawned) spawned.pal = 0;
			}

			for (int x = 0; x < 8; x++)
			{
				let a = frandom(0, 360);
				let vel = frandom(0, 4) + 4;
				let zvel = -frandom(0, 16) - self.vel.Z * 0.25;

				let spawned = dlevel.SpawnActor(self.sector, self.pos.plusZ(-8), "DukeScrap", -8, (0.75, 0.75), a, vel, zvel, self, STAT_MISC);
				if (spawned)
				{
					spawned.spriteextra = DukeScrap.Scrap3 + random(0, 3);
					spawned.pal = 6;
				}
			}
			self.counter = -3;
			self.Destroy();
			return;
		}
		// All weap
		if (self.counter == -1) //Shrinking down
		{
			self.makeitfall();

			self.cstat &= ~CSTAT_SPRITE_YFLIP;
			self.setSpriteSetImage(4);

			if (self.scale.X > 0.5 ) self.scale.X -= random(0, 7) * REPEAT_SCALE;
			if (self.scale.Y > 0.25 ) self.scale.Y -= random(0, 7) * REPEAT_SCALE;
			else
			{
				self.scale = (0.625, 0.25);
				self.counter = 0;
			}

			return;
		}
		else if (self.counter != -2) self.getglobalz();

		if (self.counter == -2) //On top of somebody (an enemy)
		{
			DukeActor s5 = self.temp_actor;
			self.makeitfall();
			if (s5)
			{
				s5.vel.X = 0;
				
				self.pos = s5.pos + s5.angle.ToVector() * 0.5;
				self.setspriteSetImage(2 + (Duke.global_random() & 1));

				if (self.scale.Y < 1) self.scale.Y += (0.03125);
				else
				{
					if (self.scale.X < 0.5) self.scale.X += (0.0625);
					else
					{
						self.counter = -1;
						self.temp_actor = nullptr;
						double dist = (self.pos.XY - s5.pos.XY).LengthSquared();
						if (dist < 48*48) {
							s5.scale.X = 0;
						}
					}
				}
			}
			else
			{
				self.temp_data[0] = -1;
			}
			return;
		}

		//Check randomly to see of there is an actor near
		if (Duke.rnd(32))
		{
			DukeSectIterator it;
			for (let a2 = it.First(self.sector); a2; a2 = it.Next())
			{
				if (a2.bGREENSLIMEFOOD)
				{
					double dist = (self.pos.XY - a2.pos.XY).LengthSquared();
					if (dist < 48*48 && (abs(self.pos.Z - a2.pos.Z) < 16)) //Gulp them
					{
						self.temp_actor = a2;
						self.counter = -2;
						self.temp_data[1] = 0;
						return;
					}
				}
			}
		}

		//Moving on the ground or ceiling

		if (self.counter == 0 || self.counter == 2)
		{
			self.setspriteSetImage(0);

			if (random(0, 511) == 0)
				self.PlayActorSound("SLIM_ROAM");

			if (self.counter == 2)
			{
				self.vel.Z = 0;
				self.cstat &= ~CSTAT_SPRITE_YFLIP;

				if ((sectp.ceilingstat & CSTAT_SECTOR_SKY) || (self.ceilingz + 24) < self.pos.Z)
				{
					self.pos.Z += 8;
					self.counter = 3;
					return;
				}
			}
			else
			{
				self.cstat |= CSTAT_SPRITE_YFLIP;
				self.makeitfall();
			}

			if (PlayClock & 4) self.DoMove(CLIPMASK0);

			if (self.vel.X > 6)
			{
				self.vel.X -= 1/8.;
				return;
			}
			else
			{
				if (self.vel.X < 2) self.vel.X += 0.25;
				self.vel.X = 4 - Raze.BobVal(512 + self.temp_data[1]) * 2;
				self.angle += deltaangle(self.angle, (pactor.pos.XY - self.pos.XY).Angle()) * 0.125;
				// TJR
			}

			self.scale.X = (0.5625 + Raze.BobVal(512 + self.temp_data[1]) * 0.125);
			self.scale.Y = (0.25 + Raze.BobVal(self.temp_data[1]) * 0.03125);

			if (Duke.rnd(4) && (sectp.ceilingstat & CSTAT_SECTOR_SKY) == 0 &&
				abs(self.floorz - self.ceilingz) < 192)
			{
				self.vel.Z = 0;
				self.counter++;
			}

		}

		if (self.counter == 1)
		{
			self.setspriteSetImage(0);
			if (self.scale.Y < 0.625) self.scale.Y += (0.125);
			if (self.scale.X > 0.125 ) self.scale.X += (-0.0625);
			if (self.vel.Z > -12)
				self.vel.Z -= 348 / 256.;
			self.pos.Z += self.vel.Z;
			if (self.pos.Z < self.ceilingz + 16)
			{
				self.pos.Z = self.ceilingz + 16;
				self.vel.X = 0;
				self.counter = 2;
			}
		}

		if (self.counter == 3)
		{
			self.setspriteSetImage(1);
			self.makeitfall();

			if (self.pos.Z > self.floorz - 8)
			{
				self.scale.Y += (-0.0625);
				self.scale.X += (0.03125);
			}
			else
			{
				if (self.scale.Y < 0.5625) self.scale.Y += (0.125);
				if (self.scale.X > 0.125 ) self.scale.X += (-0.0625);
			}

			if (self.pos.Z > self.floorz - 8)
			{
				self.pos.Z = self.floorz - 8;
				self.counter = 0;
				self.vel.X = 0;
			}
		}
	}
}
