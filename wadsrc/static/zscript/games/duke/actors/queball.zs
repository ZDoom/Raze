class DukePoolPocket : DukeActor
{
	// we only need this for checking, it's an empty sprite.
}

class DukeQueball : DukeActor
{
	default
	{
		clipdist 2;
		pic "QUEBALL";
		statnum STAT_ZOMBIEACTOR;
	}
	
	override void Initialize()
	{
		self.cstat = CSTAT_SPRITE_BLOCK_HITSCAN;
	}
	
	override void Tick()
	{
		if(self.vel.X != 0)
		{
			DukeStatIterator it;
			for(let aa = it.First(STAT_DEFAULT); aa; aa = it.Next())
			{
				double dist = (aa.pos.XY - self.pos.XY).Length();
				if (aa is 'DukePoolPocket' && dist < 3.25)
				{
					self.Destroy();
					return;
				}
			}

			CollisionData colli;
			let move = self.angle.ToVector() * self.vel.X * 0.5;
			[self.sector, self.pos] = Raze.clipmove(self.pos, self.sector, move, 1.5, 4., 4., CLIPMASK1, colli);
			int j = colli.type;

			if (j == kHitWall)
			{
				let ang = colli.hitWall().delta().Angle();
				self.angle = ang * 2 - self.angle;
			}
			else if (j == kHitSprite)
			{
				self.checkhitsprite(DukeActor(colli.hitactor()));
			}

			self.vel.X -= 1/16.;
			if(self.vel.X < 0) self.vel.X = 0;
			if (self is 'DukeStripeBall')
			{
				self.cstat = CSTAT_SPRITE_BLOCK_ALL;
				self.cstat |= (CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP) & int(self.vel.X * 16.); // special hack edition...
			}
		}
		else
		{
			double x;
			DukePlayer p;
			[p, x] = self.findplayer();

			if (x < 99.75)
			{
				//						if(self.pal == 12)
				{
					let delta = absangle(p.actor.angle, (self.pos.XY - p.actor.pos.XY).Angle());
					if (delta < 11.25 && p.PlayerInput(Duke.SB_OPEN))
						if (p.toggle_key_flag == 1)
						{
							DukeStatIterator it;
							DukeActor act2;
							for (act2 = it.First(STAT_ACTOR); act2; act2 = it.Next())
							{
								if (act2 is 'DukeQueball')
								{
									delta = absangle(p.Actor.angle, (act2.pos.XY - p.Actor.pos.XY).Angle());
									if (delta < 11.25)
									{
										double l;
										DukePlayer q;
										[q, l] = act2.findplayer();
										if (x > l) break;
									}
								}
							}
							if (act2 == nullptr)
							{
								if (self.pal == 12)
									self.vel.X = 10.25;
								else self.vel.X = 8.75;
								self.angle = p.Actor.angle;
								p.toggle_key_flag = 2;
							}
						}
				}
			}
			if (x < 32 && self.sector == p.cursector)
			{
				self.angle = (self.pos.XY - p.Actor.pos.XY).Angle();
				self.vel.X = 3;
			}
		}
	}

	override void onHit(DukeActor hitter)
	{
		if (hitter is 'DukeQueball')
		{
			hitter.vel.X = self.vel.X * 0.75;
			hitter.angle -= Normalize180(self.angle) * 2 + 180;
			self.angle = (self.pos.XY - hitter.pos.XY).Angle() - 90;
			self.PlayActorSound("POOLBALLHIT");
		}
		else
		{
			if (random(0, 3))
			{
				self.vel.X = 10.25;
				self.angle = hitter.angle;
			}
			else
			{
				self.lotsofglass(3);
				self.Destroy();
			}
		}
	}
}

class DukeStripeball : DukeQueball
{
	default
	{
		pic "STRIPEBALL";
	}
}
