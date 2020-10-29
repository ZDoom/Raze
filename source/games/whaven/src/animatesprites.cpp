#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

Loc oldLocs[MAXSPRITES];	

void analyzesprites(PLAYER& plr, int dasmoothratio) 
{
	int k;
	tspritelistcnt = spritesortcnt;
	for (int i = spritesortcnt - 1; i >= 0; i--) {
		SPRITE& tspr = tsprite[i];

		if (plr.nightglowtime <= 0) {
			if(((tspr.detail != GONZOTYPE && tspr.picnum != GONZOGSHDEAD) || tspr.shade != 31) && tspr.statnum != EVILSPIRIT)
				tspr.shade = sector[tspr.sectnum].floorshade;
		}

		auto& oldLoc = oldLocs[tspr.owner];
		// only interpolate certain moving things
		if ((tspr.hitag & 0x0200) == 0) {
			int x = oldLoc.x;
			int y = oldLoc.y;
			int z = oldLoc.z;
			short nAngle = oldLoc.ang;

			// interpolate sprite position
			x += mulscale(tspr.x - oldLoc.x, dasmoothratio, 16);
			y += mulscale(tspr.y - oldLoc.y, dasmoothratio, 16);
			z += mulscale(tspr.z - oldLoc.z, dasmoothratio, 16);
			nAngle += mulscale(((tspr.ang - oldLoc.ang + 1024) & 2047) - 1024, dasmoothratio, 16);

			tspr.x = x;
			tspr.y = y;
			tspr.z = z;
			tspr.ang = nAngle;
		}

		switch (sprite[tspr.owner].detail) {
		case GRONTYPE:
			if (tspr.picnum == GRONHAL || tspr.picnum == GRONSW || tspr.picnum == GRONSWATTACK
					|| tspr.picnum == GRONMU) {
				k = getangle(tspr.x - plr.x, tspr.y - plr.y);
				k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
				if (k <= 4) {
					tspr.picnum += (k << 2);
					tspr.cstat &= ~4; // clear x-flipping bit
				} else {
					tspr.picnum += ((8 - k) << 2);
					tspr.cstat |= 4; // set x-flipping bit
				}
			} else {
				switch (tspr.picnum) {
				case WH1GRONMUATTACK:
				case WH2GRONMUATTACK:
					if (tspr.picnum != GRONMUATTACK)
						break;
					k = getangle(tspr.x - plr.x, tspr.y - plr.y);
					k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
					if (k <= 4) {
						tspr.picnum += (k * 6);
						tspr.cstat &= ~4; // clear x-flipping bit
					} else {
						tspr.picnum += ((8 - k) * 6);
						tspr.cstat |= 4; // set x-flipping bit
					}
					break;
				case WH1GRONHALATTACK:
				case WH2GRONHALATTACK:
					if (tspr.picnum != GRONHALATTACK)
						break;
					k = getangle(tspr.x - plr.x, tspr.y - plr.y);
					k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
					if (k <= 4) {
						tspr.picnum += (k * 7);
						tspr.cstat &= ~4; // clear x-flipping bit
					} else {
						tspr.picnum += ((8 - k) * 7);
						tspr.cstat |= 4; // set x-flipping bit
					}
					break;
				}
			}

			continue;
		case GOBLINTYPE:
			if (isWh2())
				continue;

			switch (tspr.picnum) {
			case GOBLINSTAND:
			case GOBLIN:
			case GOBLINATTACK:
				k = getangle(tspr.x - plr.x, tspr.y - plr.y);
				k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
				if (k <= 4) {
					tspr.picnum += (k << 2);
					tspr.cstat &= ~4; // clear x-flipping bit
				} else {
					tspr.picnum += ((8 - k) << 2);
					tspr.cstat |= 4; // set x-flipping bit
				}
				break;
			}
			continue;
		case IMPTYPE:
			if (!isWh2())
				continue;

			k = getangle(tspr.x - plr.x, tspr.y - plr.y);
			k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
			if (tspr.picnum == IMP) {
				if (k <= 4) {
					tspr.picnum += (k * 6);
					tspr.cstat &= ~4; // clear x-flipping bit
				} else {
					tspr.picnum += ((8 - k) * 6);
					tspr.cstat |= 4; // set x-flipping bit
				}
			}
			continue;
		}

		switch (tspr.picnum) {
		case MONSTERBALL:
		case EXPLOSION:
		case PLASMA:
		case ICECUBE:
		case BULLET:
		case DISTORTIONBLAST:
		case WH1WILLOW:
		case WH2WILLOW:
			if ((tspr.picnum == WH1WILLOW && isWh2()) || (tspr.picnum == WH2WILLOW && !isWh2()))
				break;
			tspr.shade = -128;
			break;
		case DEVILSTAND:
		case DEVIL:
		case DEVILATTACK:
		case SKULLY:
		case FATWITCH:
		case JUDY:
		case FREDSTAND:
		case FRED:
		case FREDATTACK:
		case MINOTAUR:
		case MINOTAURATTACK:
			k = getangle(tspr.x - plr.x, tspr.y - plr.y);
			k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
			if (k <= 4) {
				tspr.picnum += (k << 2);
				tspr.cstat &= ~4; // clear x-flipping bit
			} else {
				tspr.picnum += ((8 - k) << 2);
				tspr.cstat |= 4; // set x-flipping bit
			}
			break;
		case WH2SKELETON:
		case WH2SKELETONATTACK:
			if (isWh2()) {
				k = getangle(tspr.x - plr.x, tspr.y - plr.y);
				k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
				if (k <= 4) {
					tspr.picnum += (k * 6);
					tspr.cstat &= ~4; // clear x-flipping bit
				} else {
					tspr.picnum += ((8 - k) * 6);
					tspr.cstat |= 4; // set x-flipping bit
				}
			}
			break;
		case WH1SKELETON:
		case WH1SKELETONATTACK:
		case KOBOLD:
		case KURTAT:
			k = getangle(tspr.x - plr.x, tspr.y - plr.y);
			k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
			if (k <= 4) {
				tspr.picnum += (k * 5);
				tspr.cstat &= ~4; // clear x-flipping bit
			} else {
				tspr.picnum += ((8 - k) * 5);
				tspr.cstat |= 4; // set x-flipping bit
			}
			break;

		case KATIE:
		case KATIEAT:

			k = getangle(tspr.x - plr.x, tspr.y - plr.y);
			k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
			if (k <= 4) {
				tspr.picnum += (k * 5);
				// tspr.cstat &= ~4; //clear x-flipping bit
				tspr.cstat |= 4; // set x-flipping bit
			} else {
				tspr.picnum += ((8 - k) * 5);
				// tspr.cstat |= 4; //set x-flipping bit
				tspr.cstat &= ~4; // clear x-flipping bit
			}
			break;

		case NEWGUY:
		case NEWGUYBOW:
		case NEWGUYMACE:

		case GONZOCSW:
		case GONZOCSWAT:
			k = getangle(tspr.x - plr.x, tspr.y - plr.y);
			k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
			if (k <= 4) {
				tspr.picnum += (k * 6);
				tspr.cstat &= ~4; // clear x-flipping bit
			} else {
				tspr.picnum += ((8 - k) * 6);
				tspr.cstat |= 4; // set x-flipping bit
			}
			break;

		case GONZOGSW:
		case GONZOGHM:
			k = getangle(tspr.x - plr.x, tspr.y - plr.y);
			k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
			if (k <= 4) {
				tspr.picnum += (k * 6);
				// tspr.cstat &= ~4; //clear x-flipping bit
				tspr.cstat |= 4; // set x-flipping bit
			} else {
				tspr.picnum += ((8 - k) * 6);
				// tspr.cstat |= 4; //set x-flipping bit
				tspr.cstat &= ~4; // clear x-flipping bit

			}
			break;

		case GONZOGSH:
			k = getangle(tspr.x - plr.x, tspr.y - plr.y);
			k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;

			tspr.picnum += (k * 6);
			tspr.cstat |= 4; // set x-flipping bit
			break;

		case NEWGUYCAST:
		case NEWGUYPUNCH:
		case KURTPUNCH:
			k = getangle(tspr.x - plr.x, tspr.y - plr.y);
			k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
			if (k <= 4) {
				tspr.picnum += (k * 3);
				tspr.cstat &= ~4; // clear x-flipping bit
			} else {
				tspr.picnum += ((8 - k) * 3);
				tspr.cstat |= 4; // set x-flipping bit
			}
			break;

		case WH1RAT:
		case WH2RAT:
			if ((tspr.picnum == WH1RAT && isWh2()) || (tspr.picnum == WH2RAT && !isWh2()))
				break;
			k = getangle(tspr.x - plr.x, tspr.y - plr.y);
			k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
			if (k <= 4) {
				tspr.picnum += (k * 2);
				tspr.cstat &= ~4; // clear x-flipping bit
			} else {
				tspr.picnum += ((8 - k) * 2);
				tspr.cstat |= 4; // set x-flipping bit
			}
			break;
		case SPIDER:
			k = getangle(tspr.x - plr.x, tspr.y - plr.y);
			k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
			if (k <= 4) {
				tspr.picnum += (k << 3);
				tspr.cstat &= ~4; // clear x-flipping bit
			} else {
				tspr.picnum += ((8 - k) << 3);
				tspr.cstat |= 4; // set x-flipping bit
			}
			break;
		case NEWGUYSTAND:
		case NEWGUYKNEE:
		case KURTSTAND:
		case KURTKNEE:
		case WH1GUARDIAN:
		case WH2GUARDIAN:
			if ((tspr.picnum == WH1GUARDIAN && isWh2()) || (tspr.picnum == WH2GUARDIAN && !isWh2()))
				break;

			k = getangle(tspr.x - plr.x, tspr.y - plr.y);
			k = (((tspr.ang + 3072 + 128 - k) & 2047) >> 8) & 7;
			if (k <= 4) {
				tspr.picnum += k;
				tspr.cstat &= ~4; // clear x-flipping bit
			} else {
				tspr.picnum += (8 - k);
				tspr.cstat |= 4; // set x-flipping bit
			}
			break;

		// ITEMS

		case SILVERBAG:
		case SILVERCOINS:
		case GOLDBAG:
		case GOLDBAG2:
		case GOLDCOINS:
		case GOLDCOINS2:
		case GIFTBOX:
		case FLASKBLUE:
		case FLASKRED:
		case FLASKGREEN:
		case FLASKOCHRE:
		case FLASKTAN:
		case DIAMONDRING:
		case SHADOWAMULET:
		case GLASSSKULL:
		case AHNK:
		case BLUESCEPTER:
		case YELLOWSCEPTER:
		case ADAMANTINERING:
		case ONYXRING:
		case SAPHIRERING:
		case WALLBOW:
		case WALLSWORD:
		case WEAPON3A:
		case WEAPON3:
		case WALLAXE:
		case GONZOBSHIELD:
		case GONZOCSHIELD:
		case GONZOGSHIELD:
		case WALLPIKE:
			tspr.shade -= 16;
			break;
		default:
			int p = tspr.picnum;
			if (p == CRYSTALSTAFF || p == AMULETOFTHEMIST || p == HORNEDSKULL || p == THEHORN || p == HELMET
					|| p == PLATEARMOR || p == CHAINMAIL || p == LEATHERARMOR || p == BRASSKEY || p == BLACKKEY
					|| p == GLASSKEY || p == IVORYKEY || p == SCROLLSCARE || p == SCROLLNIGHT || p == SCROLLFREEZE
					|| p == SCROLLMAGIC || p == SCROLLOPEN || p == SCROLLFLY || p == SCROLLFIREBALL
					|| p == SCROLLNUKE || p == QUIVER || p == BOW || p == WEAPON1 || p == WEAPON1A || p == GOBWEAPON
					|| p == WEAPON2 || p == WEAPON4 || p == THROWHALBERD || p == WEAPON5 || p == SHIELD
					|| p == WEAPON5B || p == THROWPIKE || p == WEAPON6 || p == WEAPON7 || p == PENTAGRAM) {
				tspr.shade -= 16;
				if (p == PENTAGRAM) {
					if (sector[tspr.sectnum].lotag == 4002 && plr.treasure[TPENTAGRAM] == 0)
						tspr.cstat |= 514;
				}
			}
			break;
		}
		
		if(tspr.detail != 0 && tspr.statnum != 99 && (tspr.detail & 0xFF) != SPIKEBLADETYPE) {
			if (spritesortcnt < (MAXSPRITESONSCREEN - 2)) {
				int fz = getflorzofslope(tspr.sectnum, tspr.x, tspr.y);
				if (fz > plr.z) {
					short siz = (short) std::max((tspr.xrepeat - ((fz - tspr.z) >> 10)), 1);
					if(siz > 4) {
						SPRITE& tshadow = tsprite[spritesortcnt];
						tshadow = tspr;
						int camangle = getangle(plr.x - tshadow.x, plr.y - tshadow.y);
						tshadow.x -= mulscale(sintable[(camangle + 512) & 2047], 100, 16);
						tshadow.y += mulscale(sintable[(camangle + 1024) & 2047], 100, 16);
						tshadow.z = fz + 1;
						tshadow.statnum = 99;
						
						tshadow.xrepeat = siz;
						tshadow.yrepeat = (short) (tspr.yrepeat >> 3);
						if (tshadow.yrepeat < 4)
							tshadow.yrepeat = 4;

						tshadow.shade = 127;
						tshadow.cstat |= 2;
						spritesortcnt++;
					}
				}
			}
		}
	}
}


END_WH_NS
