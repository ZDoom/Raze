
class LevelCompatibility : LevelPostProcessor
{
	protected void Apply(Name checksum, String mapname)
	{
		switch (checksum)
		{			
			case 'none':
				return;

			case '459c71d47b5beaa058253e162fd5a5c2':	// World Tour e5l1.map
				for(int i = 1373; i <= 1376; i++) SetSpriteSector(i, 860);	// fix bad sector in a few sprites.
				break;
				
			case 'c3bfb6a6e7cded2e5fe16cea86632d79':	// CP07
				SplitSector(33, 192, 196);				// sector bleeds into another area.
				break;

			case '24c7b1434070dbe01fa83c6a48926ed9': 	// RR E1L1.map 
				SplitSector(47, 367, 375);				// sector bleeds into another area.
				wall[4592].dragpoint((1615., -2715.));	// fix overlapping sectors
				break;
				
			case 'd7bf49213531cd2607e0459b950ac454':	// RR E2L7.map
				// need to add a sprite with picnum 11 (RRJAILDOOR) lotag = 48, hitag = 32, sector = 534
				// see premap_r.cpp, line 477.
				break;

			case '491a04a732cd5aa253703216ff2feff4':	// RR E1L2.map
				wall[3312].cstat |= CSTAT_WALL_BOTTOM_SWAP; // missing lower texture .
				break;
				
			case '4f2233ed8fb32f6a3deebc7803dbed69':	// SW $plax.map
				SplitSector(64, 281, 283);				// sector bleeds into another area.
				break;

			case '5e49c7f6c496e337d59d0c072ed1879b':	// SW $bath.map
				SplitSector(198, 1105, 1125);			// sector bleeds into another area.
				break;

			case 'b4ee363e9d15adc5c9becd01520acd23':	// SW $outpost.map
				SetSpriteLotag(442, -1);				// silence a misplaced and *very* annoying ambient sound.
				break;

			case '25d4164814f10cd71d26d52d494c4fb8':	// WT $auto.map 
				sector[152].exflags |= SECTOREX_DONTCLIP;	// workaround for sector object overlapping with an outer wall.
				break;

			case '2bac4971499306ee6c86462e6a03dae8':	// WT $volcano.map (original version)
			case '67207fb90130ad561479301c0970c7ba':	// WT $volcano.map (fixed version)
				sector[118].ceilingstat &= ~CSTAT_SECTOR_SKY;
				sector[57].ceilingstat &= ~CSTAT_SECTOR_SKY;
				sector[281].ceilingstat &= ~CSTAT_SECTOR_SKY;
				break;
				
			case '745182e393945e0d308e8e0a5ee80c3c': 	// SW Last Warrior level 4. 
				sw_serp_continue();						// Do not make the serpent's death end the game. The menu has no second episode so this needs to continue.
				break;
				
			case 'c6f9e49e397c0b424e8030abc23ac003':	// TD $shore.map
				ChangeSpriteFlags(298, CSTAT_SPRITE_YFLIP, 0);	// flip inverted keyhole
				ChangeSpriteFlags(307, CSTAT_SPRITE_YFLIP, 0);	// flip inverted keyhole
				break;

			case 'ef6331237eb36c84a4f7b9f5c3cd225d':	// TD level 10
				ChangeSpriteFlags(179, CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP, 0);	// flip the inverted card reader
				break;



			
		}
	}
}
