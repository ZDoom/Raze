#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static int weaponuseless = 0;
static boolean madeahit;
static int weapondropgoal;
static int arrowcnt, throwpikecnt;


// EG 17 Oct 2017: Backport shield toggle
int dropshieldcnt = 0;
boolean droptheshield = false;
int dahand = 0;
int weapondrop;
int snakex, snakey;

boolean checkmedusadist(int i, int x, int y, int z, int lvl) {
	int attackdist = (isWh2() ? 8192 : 1024) + (lvl << 9);

	if ((abs(x - sprite[i].x) + abs(y - sprite[i].y) < attackdist)
			&& (abs((z >> 8) - ((sprite[i].z >> 8) - (tileHeight(sprite[i].picnum) >> 1))) <= 120))
		return true;
	else
		return false;
}

static void playEnchantedSound(int sndnum)
{
	if (soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, 0, CHAN_ENCHANTED) == 0)
		playsound(sndnum, 0, 0, -1, CHAN_ENCHANTED);
}

void autoweaponchange(PLAYER& plr, int dagun) {
	if (plr.currweaponanim > 0 || dagun == plr.selectedgun || plr.currweaponflip > 0)
		return;

	plr.selectedgun = dagun;
	plr.hasshot = 0;
	plr.currweaponfired = 2; // drop weapon

	soundEngine->StopSound(CHAN_ENCHANTED);
	switch (plr.selectedgun) {
	case 0:
		weapondropgoal = 40;
		weapondrop = 0;
	case 1:
		weapondropgoal = 100;
		weapondrop = 0;
		break;
	case 2:
		if (plr.weapon[plr.selectedgun] == 3) {
			playEnchantedSound(S_FIREWEAPONLOOP);
		}
		weapondropgoal = (isWh2() ? 40 : 100);
		weapondrop = 0;
		break;
	case 3:
		if (plr.weapon[plr.selectedgun] == 3) {
			playEnchantedSound(S_ENERGYWEAPONLOOP);
		}
		weapondropgoal = 100;
		weapondrop = 0;
		break;
	case 4:
		weapondropgoal = 40;
		weapondrop = 0;
		break;
	case 5:
		if (plr.weapon[plr.selectedgun] == 3) {
			playEnchantedSound(S_ENERGYWEAPONLOOP);
		}
		weapondropgoal = 40;
		weapondrop = 0;
		break;
	case 6:
		if (plr.weapon[plr.selectedgun] == 3) {
			playEnchantedSound(S_FIREWEAPONLOOP);
		}
		weapondropgoal = 40;
		weapondrop = 0;
		if (plr.ammo[6] < 0)
			plr.ammo[6] = 0;
		break;
	case 7:
		if (plr.weapon[plr.selectedgun] == 3) {
			playEnchantedSound(S_ENERGYWEAPONLOOP);
		}
		weapondropgoal = 40;
		weapondrop = 0;
		if (plr.weapon[7] == 2) {
			if (plr.ammo[7] < 0)
				plr.ammo[7] = 0;
		}
		break;
	case 8:
//			if (plr.weapon[plr.selectedgun] == 3) {
//						playEnchantedSound(S_FIREWEAPONLOOP);
//			}
		weapondropgoal = 40;
		weapondrop = 0;
		break;
	case 9:
		weapondropgoal = 40;
		weapondrop = 0;
		break;
	}
}

void weaponchange(int snum) {
	PLAYER& plr = player[snum];
	if (plr.currweaponanim == 0 && plr.currweaponflip == 0) {
		int key = (plr.plInput.actions & SB_WEAPONMASK_BITS) / SB_FIRST_WEAPON_BIT - 1;
		if (key != -1 && key < 12) {
			if (key == 10 || key == 11) {
				int k = plr.currweapon;
				key = (key == 10 ? -1 : 1);
				while (k >= 0 && k < 10) {
					k += key;

					if (k == -1)
						k = 9;
					else if (k == 10)
						k = 0;

					if (plr.weapon[k] > 0) {
						key = k;
						break;
					}
				}
			}
			int gun = key;
			if (plr.weapon[gun] > 0) {
				if (plr.currweaponanim <= 0 && gun != plr.selectedgun && plr.currweaponflip <= 0) {
					soundEngine->StopSound(CHAN_ENCHANTED);
				}

				autoweaponchange(plr, gun);
			}
		}
	}
}

void plrfireweapon(PLAYER& plr) {
	if (plr.currweaponfired == 4) {
		if (isWh2()) {
			plr.currweapontics = wh2throwanimtics[plr.currentorb][0].daweapontics;
		} else
			plr.currweapontics = throwanimtics[plr.currentorb][0].daweapontics;
		return;
	}

	if (plr.ammo[plr.selectedgun] <= 0) {
		if (plr.currweapon == 6) {
			for (int i = 0; i < MAXWEAPONS; i++) {
				if (plr.ammo[i] > 0 && plr.weapon[i] == 1) {
					plr.selectedgun = i;
					plr.hasshot = 0;
					plr.currweaponfired = 2; // drop weapon
					weapondropgoal = 100;
					weapondrop = 0;
				}
			}
		}
		return;
	} else {
		madeahit = false;
		plr.ammo[plr.selectedgun]--;

		if (isWh2() && plr.weapon[plr.selectedgun] == 3) {
			if (plr.ammo[plr.selectedgun] == 0) {
				plr.weapon[plr.selectedgun] = plr.preenchantedweapon[plr.selectedgun];
				plr.ammo[plr.selectedgun] = plr.preenchantedammo[plr.selectedgun];
				soundEngine->StopSound(CHAN_ENCHANTED);
			}
		}

		if (plr.ammo[plr.selectedgun] <= 0 || plr.ammo[plr.selectedgun] == 10) {
			switch (plr.selectedgun) {
			case 0: // fist
				plr.ammo[0] = 9999;
				break;
			case 1: // knife
				if (plr.ammo[plr.selectedgun] == 10) {
					showmessage("Dagger is damaged", 360);
				}
				if (plr.ammo[plr.selectedgun] <= 0) {
					plr.ammo[1] = 0;
					plr.weapon[1] = 0;
					showmessage("Dagger is Useless", 360);
					weaponuseless = 1;
				}
				break;
			case 2: // short sword
				if (plr.ammo[plr.selectedgun] == 10) {
					showmessage("Short Sword is damaged", 360);
				}
				if (plr.ammo[plr.selectedgun] <= 0) {
					plr.ammo[2] = 0;
					plr.weapon[2] = 0;
					showmessage("Short Sword is Useless", 360);
					weaponuseless = 1;
				}
				break;
			case 3: // mace
				if (plr.ammo[plr.selectedgun] == 10) {
					showmessage("Morning Star is damaged", 360);
				}
				if (plr.ammo[plr.selectedgun] <= 0) {
					plr.ammo[3] = 0;
					plr.weapon[3] = 0;
					showmessage("Morning Star is Useless", 360);
					weaponuseless = 1;
				}
				break;

			case 4: // sword
				if (plr.ammo[plr.selectedgun] == 10) {
					showmessage("Sword is damaged", 360);
				}
				if (plr.ammo[plr.selectedgun] <= 0) {
					plr.ammo[4] = 0;
					plr.weapon[4] = 0;
					showmessage("Sword is Useless", 360);
					weaponuseless = 1;
				}
				break;
			case 5: // battle axe
				if (plr.ammo[plr.selectedgun] == 10) {
					showmessage("Battle axe is damaged", 360);
				}
				if (plr.ammo[plr.selectedgun] <= 0) {
					plr.ammo[5] = 0;
					plr.weapon[5] = 0;
					showmessage("Battle axe is Useless", 360);
					weaponuseless = 1;
				}
				break;
			case 6: // bow
				break;
			case 7: // pike
				if (plr.weapon[7] == 1) {
					if (plr.ammo[plr.selectedgun] == 10) {
						showmessage("Pike is damaged", 360);
					}
					if (plr.ammo[plr.selectedgun] <= 0) {
						plr.ammo[7] = 0;
						plr.weapon[7] = 0;
						showmessage("Pike is Useless", 360);
						weaponuseless = 1;
					}
				}
				if (plr.weapon[7] == 2 && plr.ammo[7] <= 0) {
					plr.weapon[7] = 1;
					plr.ammo[7] = 30;
				}
				break;
			case 8: // two handed sword
				if (plr.ammo[plr.selectedgun] == 10) {
					showmessage("Magic Sword is damaged", 360);
				}
				if (plr.ammo[plr.selectedgun] <= 0) {
					plr.ammo[8] = 0;
					plr.weapon[8] = 0;
					showmessage("Magic Sword is Useless", 360);
					weaponuseless = 1;
				}
				break;
			case 9: // halberd
				if (plr.ammo[plr.selectedgun] == 10) {
					showmessage("Halberd is damaged", 360);
				}
				if (plr.ammo[plr.selectedgun] <= 0) {
					plr.ammo[9] = 0;
					plr.weapon[9] = 0;
					showmessage("Halberd is Useless", 360);
					weaponuseless = 1;
				}
				break;
			}
		}
	}

	if (weaponuseless == 1)
		for (int i = 0; i < MAXWEAPONS; i++) {
			if (plr.weapon[i] > 0 && plr.ammo[i] > 0) {
				plr.currweapon = plr.selectedgun = i;
				plr.currweaponfired = 3; // ready weapon
				plr.currweaponflip = 0;
				weaponuseless = 0;
			}
		}
	else
		plr.currweaponfired = 1;

	plr.currweapon = plr.selectedgun;

	plr.currweaponattackstyle = krand() % 2;

	if (plr.weapon[7] == 2 && plr.currweapon == 7) {
		plr.currweaponattackstyle = 0;
	} else if (isWh2() && plr.weapon[7] == 3 && plr.currweapon == 7) {
		plr.currweaponattackstyle = 0;
	}

	if (plr.currweapon == 9) {
		if (krand() % 100 > 80)
			plr.currweaponattackstyle = 0;
		else
			plr.currweaponattackstyle = 1;
	}

	if (plr.currweaponanim > 11) {
		if (isWh2()) {
			if (plr.weapon[plr.currweapon] == 1)
				plr.currweapontics = weaponanimtics[plr.currweapon][0].daweapontics;
			else
				plr.currweapontics = zweaponanimtics[plr.currweapon][0].daweapontics;
		} else
			plr.currweapontics = weaponanimtics[plr.currweapon][0].daweapontics;
	}
}

void weaponsprocess(int snum) {
	PLAYER& plr = player[snum];

	if (plr.shadowtime <= 0) {
		if (plr.weapon[plr.currweapon] == 3) {
			if (plr.weapon[plr.selectedgun] == 3) {
				switch (plr.selectedgun) {
				case 2:
				case 6:
					playEnchantedSound(S_FIREWEAPONLOOP);
					break;
				default:
					playEnchantedSound(S_ENERGYWEAPONLOOP);
					break;
				}
			}
		}
	}

	if (plr.currweapon == 0 && dahand == 0)
		if (krand() % 2 == 0)
			dahand = 1;
		else
			dahand = 2;

	switch (plr.currweaponfired) {
	case 6:
		switch (plr.currweapon) {
		case 1: // knife
			if (plr.currweaponframe == KNIFEATTACK2 + 1)
				if ((plr.currweaponanim == 2 || plr.currweaponanim == 10) && plr.currweapontics == 8)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		case 3: // morning
			if (plr.currweaponframe == MORNINGATTACK2 + 3)
				if (plr.currweaponanim == 3 && plr.currweapontics == 12)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		}

		if (plr.currweaponframe == RFIST + 5 || plr.currweaponframe == KNIFEATTACK + 6
				|| plr.currweaponframe == ZKNIFEATTACK + 5 // new
				|| plr.currweaponframe == MORNINGSTAR + 5 || plr.currweaponframe == SWORDATTACK + 7
				|| plr.currweaponframe == BOWWALK + 5 || plr.currweaponframe == ZBOWATTACK + 4
				|| plr.currweaponframe == KNIFEATTACK2 + 2 || plr.currweaponframe == ZKNIFEATTACK2 + 2
				|| plr.currweaponframe == SWORDATTACK2 + 6 || plr.currweaponframe == MORNINGATTACK2 + 3
				|| plr.currweaponframe == HALBERDATTACK1 + 3 || plr.currweaponframe == HALBERDATTACK2 + 3
				|| plr.currweaponframe == BIGAXEATTACK + 7 || plr.currweaponframe == BIGAXEATTACK2 + 6
				|| plr.currweaponframe == PIKEATTACK1 + 4 || plr.currweaponframe == PIKEATTACK2 + 4
				|| plr.currweaponframe == EXCALATTACK1 + 7 || plr.currweaponframe == EXCALATTACK2 + 5
				|| plr.currweaponframe == GOBSWORDATTACK2 + 4 || plr.currweaponframe == GOBSWORDATTACK + 4
				|| plr.currweaponframe == ZSHORTATTACK + 7 || plr.currweaponframe == ZSHORTATTACK2 + 4
				|| plr.currweaponframe == ZSTARATTACK + 7 || plr.currweaponframe == ZSTARATTACK2 + 3
				|| plr.currweaponframe == ZAXEATTACK + 12 || plr.currweaponframe == ZAXEATTACK2 + 6
				|| plr.currweaponframe == ZPIKEATTACK + 4 || plr.currweaponframe == ZPIKEATTACK2 + 4
				|| plr.currweaponframe == ZTWOHANDATTACK + 12 || plr.currweaponframe == ZTWOHANDATTACK2 + 5
				|| plr.currweaponframe == ZHALBERDATTACK + 4 || plr.currweaponframe == ZHALBERDATTACK2 + 3)

			swingdaweapon(plr);

		plr.currweapontics -= TICSPERFRAME;
		if (plr.helmettime > 0)
			plr.currweapontics--;

		if (plr.currweapontics < 0) {
			plr.currweaponanim++;

			if (plr.currweaponanim > 11) {
				plr.currweaponanim = 0;
				plr.currweaponfired = 0;
				plr.currweaponflip = 0;
				plr.currweapon = plr.selectedgun;
				if (dahand > 0)
					dahand = 0;
			}

			if (isWh2()) {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
					plr.currweapontics = lefthandanimtics[plr.currweapon][plr.currweaponanim].daweapontics;
					plr.currweaponframe = lefthandanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
				} else {
					plr.currweapontics = zlefthandanimtics[plr.currweapon][plr.currweaponanim].daweapontics;
					plr.currweaponframe = zlefthandanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;

				}
			} else {
				plr.currweapontics = lefthandanimtics[plr.currweapon][plr.currweaponanim].daweapontics;
				plr.currweaponframe = lefthandanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
			}
		} else {
			if (isWh2()) {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
					plr.currweaponframe = lefthandanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
				} else {
					plr.currweaponframe = zlefthandanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
				}
			} else
				plr.currweaponframe = lefthandanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
		}

		if (plr.currweapon == 0 && plr.currweaponframe == 0) {
			dahand = 0;
			plr.currweaponanim = 0;
			plr.currweaponfired = 0;
		}

		if (plr.selectedgun == 4 && plr.currweaponframe == 0) {
			plr.currweaponanim = 0;
			plr.currweaponfired = 0;
			plr.currweaponflip = 0;
			plr.currweapon = plr.selectedgun;
		}
		break;
	case 1: // fire
		switch (plr.currweapon) {
		case 0: // fist
			if (plr.currweaponframe == RFIST + 5)
				if (plr.currweaponanim == 5 && plr.currweapontics == 10)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		case 1: // knife
			if (plr.currweaponframe == KNIFEATTACK + 6 || plr.currweaponframe == ZKNIFEATTACK + 5)
				if (plr.currweaponanim == 8 && plr.currweapontics == 8)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			if (plr.currweaponframe == KNIFEATTACK2 + 2 || plr.currweaponframe == ZKNIFEATTACK2 + 2)
				if ((plr.currweaponanim == 5 || plr.currweaponanim == 9) && plr.currweapontics == 8)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		case 2: // shortsword
			if (plr.currweaponframe == GOBSWORDATTACK + 4 || plr.currweaponframe == ZSHORTATTACK + 7)
				if (plr.currweaponanim == 4 && plr.currweapontics == 10)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			if (plr.currweaponframe == GOBSWORDATTACK2 + 4 || plr.currweaponframe == ZSHORTATTACK2 + 4)
				if (plr.currweaponanim == 4 && plr.currweapontics == 10)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		case 3: // morning
			if (plr.currweaponframe == MORNINGSTAR + 5 || plr.currweaponframe == ZSTARATTACK + 7)
				if (plr.currweaponanim == 7 && plr.currweapontics == 12)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			if (plr.currweaponframe == MORNINGATTACK2 + 3 || plr.currweaponframe == ZSTARATTACK2 + 3)
				if (plr.currweaponanim == 3 && plr.currweapontics == 12)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		case 4: // sword
			if (plr.currweaponframe == SWORDATTACK + 7)
				if (plr.currweaponanim == 7 && plr.currweapontics == 8)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			if (plr.currweaponframe == SWORDATTACK2 + 6)
				if (plr.currweaponanim == 6 && plr.currweapontics == 8)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		case 5: // battleaxe
			if (plr.currweaponframe == BIGAXEATTACK + 7 || plr.currweaponframe == ZAXEATTACK + 12)
				if (plr.currweaponanim == 7 && plr.currweapontics == 12)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			if (plr.currweaponframe == BIGAXEATTACK2 + 6 || plr.currweaponframe == ZAXEATTACK2 + 6)
				if (plr.currweaponanim == 6 && plr.currweapontics == 12)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		case 6: // bow
			if (plr.currweaponframe == BOWWALK + 4)
				if (plr.currweaponanim == 4 && plr.currweapontics == 6)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			if (plr.currweaponframe == ZBOWATTACK + 4)
				if (plr.currweaponanim == 4 && plr.currweapontics == 6)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		case 7: // pike
			if (plr.currweaponframe == PIKEATTACK1 + 4 || plr.currweaponframe == ZPIKEATTACK + 4)
				if (plr.currweaponanim == 8 && plr.currweapontics == 10)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			if (plr.currweaponframe == PIKEATTACK2 + 4 || plr.currweaponframe == ZPIKEATTACK2 + 4)
				if (plr.currweaponanim == 4 && plr.currweapontics == 10)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		case 8: // two handed sword
			if (plr.currweaponframe == EXCALATTACK1 + 7 || plr.currweaponframe == ZTWOHANDATTACK + 12)
				if (plr.currweaponanim == 7 && plr.currweapontics == 8)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			if (plr.currweaponframe == EXCALATTACK2 + 5 || plr.currweaponframe == ZTWOHANDATTACK2 + 5)
				if (plr.currweaponanim == 5 && plr.currweapontics == 8)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		case 9: // halberd
			if (plr.currweaponframe == HALBERDATTACK1 + 3 || plr.currweaponframe == ZHALBERDATTACK + 4)
				if (plr.currweaponanim == 7 && plr.currweapontics == 12)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			if (plr.currweaponframe == HALBERDATTACK2 + 3 || plr.currweaponframe == ZHALBERDATTACK2 + 3)
				if (plr.currweaponanim == 4 && plr.currweapontics == 12)
					swingdasound(plr.currweapon, plr.weapon[plr.currweapon] == 3);
			break;
		}

		if (plr.currweaponframe == RFIST + 5 || plr.currweaponframe == KNIFEATTACK + 6
				|| plr.currweaponframe == ZKNIFEATTACK + 5 // new
				|| plr.currweaponframe == MORNINGSTAR + 5 || plr.currweaponframe == SWORDATTACK + 7
				|| plr.currweaponframe == BOWWALK + 5 || plr.currweaponframe == ZBOWATTACK + 4
				|| plr.currweaponframe == KNIFEATTACK2 + 2 || plr.currweaponframe == ZKNIFEATTACK2 + 2
				|| plr.currweaponframe == SWORDATTACK2 + 6 || plr.currweaponframe == MORNINGATTACK2 + 3
				|| plr.currweaponframe == HALBERDATTACK1 + 3 || plr.currweaponframe == HALBERDATTACK2 + 3
				|| plr.currweaponframe == BIGAXEATTACK + 7 || plr.currweaponframe == BIGAXEATTACK2 + 6
				|| plr.currweaponframe == PIKEATTACK1 + 4 || plr.currweaponframe == PIKEATTACK2 + 4
				|| plr.currweaponframe == EXCALATTACK1 + 7 || plr.currweaponframe == EXCALATTACK2 + 5
				|| plr.currweaponframe == GOBSWORDATTACK2 + 4 || plr.currweaponframe == GOBSWORDATTACK + 4
				|| plr.currweaponframe == ZSHORTATTACK + 7 || plr.currweaponframe == ZSHORTATTACK2 + 4
				|| plr.currweaponframe == ZSTARATTACK + 7 || plr.currweaponframe == ZSTARATTACK2 + 3
				|| plr.currweaponframe == ZAXEATTACK + 12 || plr.currweaponframe == ZAXEATTACK2 + 6
				|| plr.currweaponframe == ZPIKEATTACK + 4 || plr.currweaponframe == ZPIKEATTACK2 + 4
				|| plr.currweaponframe == ZTWOHANDATTACK + 12 || plr.currweaponframe == ZTWOHANDATTACK2 + 5
				|| plr.currweaponframe == ZHALBERDATTACK + 4 || plr.currweaponframe == ZHALBERDATTACK2 + 3)

			swingdaweapon(plr);

		plr.currweapontics -= TICSPERFRAME;
		if (plr.helmettime > 0)
			plr.currweapontics--;

		if (plr.shieldpoints <= 0)
			droptheshield = true;

		if ((plr.currweaponframe == SWORDATTACK + 7 || plr.currweaponframe == SWORDATTACK2 + 7)
				&& plr.currweapontics < 0 && droptheshield) {
			if (rand() % 100 > 50) {

				if (isWh2()) {
					if (plr.ammo[1] > 0 && plr.weapon[3] == 0) {
						plr.currweapon = 1;
						plr.currweapontics = 6;
						plr.currweaponanim = 0;
						plr.currweaponfired = 6;
						plr.hasshot = 0;
						plr.currweaponflip = 1;
					}
					if (plr.ammo[3] > 0) {
						plr.currweapon = 3;
						plr.currweapontics = 6;
						plr.currweaponanim = 0;
						plr.currweaponfired = 6;
						plr.hasshot = 0;
						plr.currweaponflip = 1;
					}
				} else {
					if (plr.lvl >= 4 && plr.lvl <= 4 && plr.ammo[1] > 0) {
						plr.currweapon = 1;
						plr.currweapontics = 6;
						plr.currweaponanim = 0;
						plr.currweaponfired = 6;
						plr.hasshot = 0;
						plr.currweaponflip = 1;
					} else if (plr.lvl >= 5 && plr.ammo[3] > 0) {
						plr.currweapon = 3;
						plr.currweapontics = 6;
						plr.currweaponanim = 0;
						plr.currweaponfired = 6;
						plr.hasshot = 0;
						plr.currweaponflip = 1;
					}
				}
			}
		}
		if (plr.currweapontics < 0) {
			plr.currweaponanim++;
			if (plr.currweaponanim > 11) {
				plr.currweaponanim = 0;
				plr.currweaponfired = 0;
				if (dahand > 0)
					dahand = 0;
			}
			if (plr.currweaponattackstyle == 0) {
				if (isWh2()) {
					if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
						plr.currweapontics = weaponanimtics[plr.currweapon][plr.currweaponanim].daweapontics;
						plr.currweaponframe = weaponanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
					} else {
						plr.currweapontics = zweaponanimtics[plr.currweapon][plr.currweaponanim].daweapontics;
						plr.currweaponframe = zweaponanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
					}
				} else {
					plr.currweapontics = weaponanimtics[plr.currweapon][plr.currweaponanim].daweapontics;
					plr.currweaponframe = weaponanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
				}
			} else {
				if (isWh2()) {
					if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
						plr.currweapontics = weaponanimtics2[plr.currweapon][plr.currweaponanim].daweapontics;
						plr.currweaponframe = weaponanimtics2[plr.currweapon][plr.currweaponanim].daweaponframe;
					} else {
						plr.currweapontics = zweaponanimtics2[plr.currweapon][plr.currweaponanim].daweapontics;
						plr.currweaponframe = zweaponanimtics2[plr.currweapon][plr.currweaponanim].daweaponframe;
					}
				} else {
					plr.currweapontics = weaponanimtics2[plr.currweapon][plr.currweaponanim].daweapontics;
					plr.currweaponframe = weaponanimtics2[plr.currweapon][plr.currweaponanim].daweaponframe;
				}
			}
		} else {
			if (isWh2()) {
				if (plr.currweaponattackstyle == 0) {
					// flip
					if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
						plr.currweaponframe = weaponanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
					} else {
						plr.currweaponframe = zweaponanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
					}
				} else {
					// flip
					if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
						plr.currweaponframe = weaponanimtics2[plr.currweapon][plr.currweaponanim].daweaponframe;
					} else {
						plr.currweaponframe = zweaponanimtics2[plr.currweapon][plr.currweaponanim].daweaponframe;
					}
				}
			} else {
				if (plr.currweaponattackstyle == 0) {
					// flip
					plr.currweaponframe = weaponanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
				} else {
					// flip
					plr.currweaponframe = weaponanimtics2[plr.currweapon][plr.currweaponanim].daweaponframe;
				}
			}
		}

		if (plr.currweapon == 0 && plr.currweaponframe == 0) {
			dahand = 0;
			plr.currweaponanim = 0;
			plr.currweaponfired = 0;
		}
		break;

	case 0: // walking
		if (isWh2()) {
			if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2)
				plr.currweapontics = weaponanimtics[plr.currweapon][0].daweapontics;
			else
				plr.currweapontics = zweaponanimtics[plr.currweapon][0].daweapontics;

			if (plr.currweapon == 6 && plr.ammo[6] <= 0) {
				// wango
				if (plr.weapon[plr.currweapon] == 1)
					plr.currweaponframe = BOWREADYEND;
				else
					plr.currweaponframe = ZBOWWALK;
			} else {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2)
					plr.currweaponframe = weaponanimtics[plr.currweapon][0].daweaponframe;
				else
					plr.currweaponframe = zweaponanimtics[plr.currweapon][0].daweaponframe;
			}
		} else {
			plr.currweapontics = weaponanimtics[plr.currweapon][0].daweapontics;

			if (plr.currweapon == 6 && plr.ammo[6] <= 0)
				plr.currweaponframe = BOWREADYEND;
			else
				plr.currweaponframe = weaponanimtics[plr.currweapon][0].daweaponframe;
		}
		if (plr.plInput.fvel || plr.plInput.svel) {
			snakex = (sintable[(lockclock << 4) & 2047] >> 12);
			snakey = (sintable[(lockclock << 4) & 2047] >> 12);
		}
		break;
	case 2: // unready
		if (plr.currweapon == 1)
			weapondrop += TICSPERFRAME << 1;
		else
			weapondrop += TICSPERFRAME;
		if (weapondrop > weapondropgoal) {
			plr.currweaponfired = 3;
//				weaponraise = 40;
			plr.currweapon = plr.selectedgun;
		}

		if (isWh2()) {
			if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2)
				plr.currweapontics = weaponanimtics[plr.currweapon][0].daweapontics;
			else
				plr.currweapontics = zweaponanimtics[plr.currweapon][0].daweapontics;

			if (plr.currweapon == 6 && plr.ammo[6] <= 0) {
				if (plr.weapon[plr.currweapon] == 1)
					plr.currweaponframe = BOWREADYEND;
				else
					// currweaponframe=ZBOWREADYEND;
					plr.currweaponframe = ZBOWWALK;
			} else {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2)
					plr.currweaponframe = weaponanimtics[plr.currweapon][0].daweaponframe;
				else
					plr.currweaponframe = zweaponanimtics[plr.currweapon][0].daweaponframe;
			}
		} else {
			plr.currweapontics = weaponanimtics[plr.currweapon][0].daweapontics;

			if (plr.currweapon == 6 && plr.ammo[6] <= 0)
				plr.currweaponframe = BOWREADYEND;
			else
				plr.currweaponframe = weaponanimtics[plr.currweapon][0].daweaponframe;
		}
		break;
	case 3: // ready
		plr.currweapontics -= TICSPERFRAME;
		if (plr.currweapontics < 0) {
			plr.currweaponanim++;
			if (plr.currweaponanim == 12) {
				plr.currweaponanim = 0;
				plr.currweaponfired = 0;

				if (isWh2()) {
					if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
						plr.currweaponframe = readyanimtics[plr.currweapon][11].daweaponframe;
					} else {
						plr.currweaponframe = zreadyanimtics[plr.currweapon][11].daweaponframe;
					}
				} else
					plr.currweaponframe = readyanimtics[plr.currweapon][11].daweaponframe;
				break;
			}
			if (isWh2()) {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
					plr.currweapontics = readyanimtics[plr.currweapon][plr.currweaponanim].daweapontics;
					plr.currweaponframe = readyanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
				} else {
					plr.currweapontics = zreadyanimtics[plr.currweapon][plr.currweaponanim].daweapontics;
					plr.currweaponframe = zreadyanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
				}
			} else {
				plr.currweapontics = readyanimtics[plr.currweapon][plr.currweaponanim].daweapontics;
				plr.currweaponframe = readyanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
			}
		} else {
			if (isWh2()) {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
					plr.currweaponframe = readyanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;

				} else {
					plr.currweaponframe = zreadyanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
				}
			} else
				plr.currweaponframe = readyanimtics[plr.currweapon][plr.currweaponanim].daweaponframe;
		}
		break;
	case 5: // cock
		plr.currweapontics -= (TICSPERFRAME);
		if (plr.currweapontics < 0) {
			plr.currweaponanim++;
			if (plr.currweaponanim == 4) {
				plr.currweaponanim = 0;
				plr.currweaponfired = 0;
				if (isWh2()) {
					if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
						plr.currweaponframe = cockanimtics[3].daweaponframe;
					} else {
						if (plr.weapon[plr.currweapon] == 3) {
							plr.currweaponframe = zcockanimtics[4].daweaponframe;
						} else {
							plr.currweaponframe = cockanimtics[4].daweaponframe;

						}
					}

				} else
					plr.currweaponframe = cockanimtics[3].daweaponframe;
				break;
			}

			if (isWh2()) {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
					plr.currweapontics = cockanimtics[plr.currweaponanim].daweapontics;
					plr.currweaponframe = cockanimtics[plr.currweaponanim].daweaponframe;
				} else {
					if (plr.weapon[plr.currweapon] == 3) {
						plr.currweapontics = zcockanimtics[plr.currweaponanim].daweapontics;
						plr.currweaponframe = zcockanimtics[plr.currweaponanim].daweaponframe;

					} else {
						plr.currweapontics = cockanimtics[plr.currweaponanim].daweapontics;
						plr.currweaponframe = cockanimtics[plr.currweaponanim].daweaponframe;
					}
				}
			} else {
				plr.currweapontics = cockanimtics[plr.currweaponanim].daweapontics;
				plr.currweaponframe = cockanimtics[plr.currweaponanim].daweaponframe;
			}
		} else {
			if (isWh2()) {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
					plr.currweaponframe = cockanimtics[plr.currweaponanim].daweaponframe;
				} else {
					if (plr.weapon[plr.currweapon] == 3) {
						plr.currweaponframe = zcockanimtics[plr.currweaponanim].daweaponframe;
					} else {
						plr.currweaponframe = zcockanimtics[plr.currweaponanim].daweaponframe;
					}
				}
			} else
				plr.currweaponframe = cockanimtics[plr.currweaponanim].daweaponframe;
		}
		break;
	case 4: // throw the orb

		if (plr.currweaponframe == 0) 
			castaorb(plr);

		plr.currweapontics -= (TICSPERFRAME);
		if (plr.currweapontics < 0) {
			plr.currweaponanim++;
			if (plr.currweaponanim > 12) {
				plr.currweaponanim = 0;
				plr.currweaponfired = 0;
				plr.orbshot = 0;

				if (isWh2()) {
					plr.currweaponframe = wh2throwanimtics[plr.currentorb][plr.currweaponanim].daweaponframe;
				} else
					plr.currweaponframe = throwanimtics[plr.currentorb][plr.currweaponanim].daweaponframe;
				break;
			}

			if (isWh2()) {
				plr.currweapontics = wh2throwanimtics[plr.currentorb][plr.currweaponanim].daweapontics;
				plr.currweaponframe = wh2throwanimtics[plr.currentorb][plr.currweaponanim].daweaponframe;
			} else {
				plr.currweapontics = throwanimtics[plr.currentorb][plr.currweaponanim].daweapontics;
				plr.currweaponframe = throwanimtics[plr.currentorb][plr.currweaponanim].daweaponframe;
			}

		} else {
			if (isWh2()) {
				plr.currweaponframe = wh2throwanimtics[plr.currentorb][plr.currweaponanim].daweaponframe;
			} else
				plr.currweaponframe = throwanimtics[plr.currentorb][plr.currweaponanim].daweaponframe;
		}
		break;
	}
		
	if(plr.currweaponfired != 4 && plr.orbammo[plr.currentorb] <= 0) 
		spellswitch(plr, 1);

	if (plr.shieldpoints > 0 && (plr.currweaponfired == 0 || plr.currweaponfired == 1) && plr.selectedgun > 0
			&& plr.selectedgun < 5 && !droptheshield) {
		if (plr.currweaponfired == 1) {
			snakex = (sintable[(lockclock << 4) & 2047] >> 12);
			snakey = (sintable[(lockclock << 4) & 2047] >> 12);
			if (droptheshield) {
				dropshieldcnt += (TICSPERFRAME << 1);
				snakey += dropshieldcnt;
			}
		}

		if (dropshieldcnt > 200) {
			dropshieldcnt = 0;
			droptheshield = true;
		}
	}
}

void madenoise(PLAYER& plr, int val, int x, int y, int z) {
	short nextsprite;
	for (short i = headspritestat[FACE]; i >= 0; i = nextsprite) {
		nextsprite = nextspritestat[i];
		if ((abs(x - sprite[i].x) + abs(y - sprite[i].y) < (val * 4096)))
			newstatus(i, FINDME);
	}
}

void shootgun(PLAYER& plr, float ang, int guntype) {
	int k = 0, daz2;
	short j, i;

	int daang = (int) ang;
		
	if(plr.hasshot == 1)
		return;

	Hitscan pHitInfo;
	switch (guntype) {
	case 0:
		daz2 = -mulscale16(plr.horizon.horiz.asq16(), 2000);

		hitscan(plr.x, plr.y, plr.z, plr.sector, // Start position
				sintable[(daang + 2560) & 2047], // X vector of 3D ang
				sintable[(daang + 2048) & 2047], // Y vector of 3D ang
				daz2, // Z vector of 3D ang
				pHitInfo, CLIPMASK1);

		if (pHitInfo.hitsprite >= 0)
			madeahit = true;

		if (pHitInfo.hitwall >= 0) {
			if ((abs(plr.x - pHitInfo.hitx) + abs(plr.y - pHitInfo.hity) < 512)
					&& (abs((plr.z >> 8) - ((pHitInfo.hitz >> 8) - (64))) <= (512 >> 3))) {
				madeahit = true;
				switch (plr.currweapon) {
				case 0: // fist
					if (plr.currweaponframe == RFIST + 5)
						if (plr.currweaponanim == 5 && plr.currweapontics == 10)
							swingdapunch(plr, plr.currweapon);
					break;
				case 1: // knife
					if (plr.currweaponframe == KNIFEATTACK + 6)
						if (plr.currweaponanim == 8 && plr.currweapontics == 8)
							swingdapunch(plr, plr.currweapon);
					if (plr.currweaponframe == KNIFEATTACK2 + 2)
						if (plr.currweaponanim == 5 || plr.currweaponanim == 9 && plr.currweapontics == 8)
							swingdapunch(plr, plr.currweapon);
					break;
				case 2: // short sword
					if (plr.currweaponframe == GOBSWORDATTACK + 4 || plr.currweaponframe == ZSHORTATTACK + 7)
						if (plr.currweaponanim == 4 && plr.currweapontics == 10)
							swingdapunch(plr, plr.currweapon);
					if (plr.currweaponframe == GOBSWORDATTACK + 4 || plr.currweaponframe == ZSHORTATTACK + 4)
						if (plr.currweaponanim == 4 && plr.currweapontics == 10)
							swingdapunch(plr, plr.currweapon);
					break;
				case 3: // morning
					if (plr.currweaponframe == MORNINGSTAR + 5 || plr.currweaponframe == ZSTARATTACK + 7)
						if (plr.currweaponanim == 7 && plr.currweapontics == 12)
							swingdapunch(plr, plr.currweapon);
					if (plr.currweaponframe == MORNINGATTACK2 + 3 || plr.currweaponframe == ZSTARATTACK + 3)
						if (plr.currweaponanim == 3 && plr.currweapontics == 12)
							swingdapunch(plr, plr.currweapon);
					break;
				case 4: // sword
					if (plr.currweaponframe == SWORDATTACK + 7)
						if (plr.currweaponanim == 7 && plr.currweapontics == 8) {
							swingdapunch(plr, plr.currweapon);
							madenoise(plr, 2, plr.x, plr.y, plr.z);
						}
					if (plr.currweaponframe == SWORDATTACK2 + 6)
						if (plr.currweaponanim == 6 && plr.currweapontics == 8) {
							swingdapunch(plr, plr.currweapon);
							madenoise(plr, 2, plr.x, plr.y, plr.z);
						}
					break;
				case 5: // battleaxe
					if (plr.currweaponframe == BIGAXEATTACK + 7 || plr.currweaponframe == ZAXEATTACK + 12)
						if (plr.currweaponanim == 7 && plr.currweapontics == 12)
							swingdapunch(plr, plr.currweapon);
					if (plr.currweaponframe == BIGAXEATTACK2 + 6 || plr.currweaponframe == ZAXEATTACK2 + 6)
						if (plr.currweaponanim == 6 && plr.currweapontics == 12)
							swingdapunch(plr, plr.currweapon);
					break;
				case 6: // bow
					if (plr.currweaponframe == BOWWALK + 4)
						if (plr.currweaponanim == 4 && plr.currweapontics == 6)
							swingdapunch(plr, plr.currweapon);
					if (plr.currweaponframe == ZBOWATTACK + 4)
						if (plr.currweaponanim == 4 && plr.currweapontics == 6)
							swingdapunch(plr, plr.currweapon);
					break;
				case 7: // pike
					if (plr.currweaponframe == PIKEATTACK1 + 4 || plr.currweaponframe == ZPIKEATTACK + 4)
						if (plr.currweaponanim == 8 && plr.currweapontics == 10)
							swingdapunch(plr, plr.currweapon);
					if (plr.currweaponframe == PIKEATTACK2 + 4 || plr.currweaponframe == ZPIKEATTACK2 + 4)
						if (plr.currweaponanim == 4 && plr.currweapontics == 10)
							swingdapunch(plr, plr.currweapon);
					break;
				case 8: // two handed sword
					if (plr.currweaponframe == EXCALATTACK1 + 7 || plr.currweaponframe == ZTWOHANDATTACK + 12)
						if (plr.currweaponanim == 7 && plr.currweapontics == 8)
							swingdapunch(plr, plr.currweapon);
					if (plr.currweaponframe == EXCALATTACK2 + 5 || plr.currweaponframe == ZTWOHANDATTACK2 + 5)
						if (plr.currweaponanim == 5 && plr.currweapontics == 8)
							swingdapunch(plr, plr.currweapon);
					break;
				case 9: // halberd
					if (plr.currweaponframe == HALBERDATTACK1 + 3 || plr.currweaponframe == ZHALBERDATTACK + 4)
						if (plr.currweaponanim == 6 && plr.currweapontics == 12)
							swingdapunch(plr, plr.currweapon);
					if (plr.currweaponframe == HALBERDATTACK2 + 3 || plr.currweaponframe == ZHALBERDATTACK2 + 3)
						if (plr.currweaponanim == 4 && plr.currweapontics == 12)
							swingdapunch(plr, plr.currweapon);
					break;
				}
			}
		}

		if (checkweapondist(pHitInfo.hitsprite, plr.x, plr.y, plr.z, plr.selectedgun)) {
			madeahit = true;

			switch (sprite[pHitInfo.hitsprite].detail) {

			case DEMONTYPE:
			case GONZOTYPE:
			case KATIETYPE:
			case KURTTYPE:
			case NEWGUYTYPE:
			case GRONTYPE:
			case KOBOLDTYPE:
			case DRAGONTYPE:
			case DEVILTYPE:
			case FREDTYPE:
			case SKELETONTYPE:
			case GOBLINTYPE:
			case IMPTYPE:
			case MINOTAURTYPE:
			case SPIDERTYPE:
			case SKULLYTYPE:
			case FATWITCHTYPE:
			case FISHTYPE:
			case RATTYPE:
			case WILLOWTYPE:
			case GUARDIANTYPE:
			case JUDYTYPE:
				if (netgame) {
					// XXX netshootgun(pHitInfo.hitsprite,currweapon);
				}
					
				if(sprite[pHitInfo.hitsprite].statnum == DIE || sprite[pHitInfo.hitsprite].statnum == DEAD) //already dying
					break;

				if (isWh2() && plr.currweapon == 3)
					if (plr.weapon[plr.currweapon] == 3) {
						explosion(pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity, pHitInfo.hitz, 4096);
					}

				if (plr.invisibletime > 0) {
					if (isWh2()) {
						if ((krand() & 32) > 15)
							plr.invisibletime = -1;
					} else {
						if ((krand() & 1) != 0)
							plr.invisibletime = -1;
					}
				}

				switch (plr.selectedgun) {
				case 0: // fist
					if (isWh2()) {
						k = (krand() % 5) + 1;
						break;
					}
					k = (krand() & 5) + 1;
					break;
				case 1: // dagger
					if (isWh2()) {
						k = (krand() % 5) + 5;
						break;
					}

					if (plr.currweaponattackstyle == 0)
						k = (krand() & 5) + 10;
					else
						k = (krand() & 3) + 5;

					break;
				case 2: // short sword
					if (isWh2()) {
						k = (krand() % 10) + 5;
						break;
					}

					if (plr.currweaponattackstyle == 0)
						k = (krand() & 10) + 10;
					else
						k = (krand() & 6) + 10;
					break;
				case 3: // morning star
					if (isWh2()) {
						k = (krand() % 15) + 5;
						break;
					}

					if (plr.currweaponattackstyle == 0)
						k = (krand() & 8) + 10;
					else
						k = (krand() & 8) + 15;
					break;
				case 4: // broad sword
					if (isWh2()) {
						k = (krand() % 20) + 5;
						break;
					}

					if (plr.currweaponattackstyle == 0)
						k = (krand() & 5) + 20;
					else
						k = (krand() & 5) + 15;
					break;
				case 5: // battle axe
					if (isWh2()) {
						k = (krand() % 25) + 5;
						switch (sprite[pHitInfo.hitsprite].detail) {
						case GRONTYPE:
						case NEWGUYTYPE:
						case KURTTYPE:
						case GONZOTYPE:
							k += k >> 1;
							break;
						}
						break;
					}

					if (plr.currweaponattackstyle == 0)
						k = (krand() & 5) + 25;
					else
						k = (krand() & 5) + 20;
					break;
				case 6: // bow
					if (isWh2()) {
						k = (krand() % 30) + 5;
						break;
					}

					if (plr.currweaponattackstyle == 0)
						k = (krand() & 15) + 5;
					else
						k = (krand() & 15) + 5;
					break;
				case 7: // pike axe
					if (isWh2()) {
						k = (krand() % 35) + 5;
						break;
					}
					if (plr.currweaponattackstyle == 0)
						k = (krand() & 15) + 10;
					else
						k = (krand() & 15) + 5;
					break;
				case 8: // two handed sword
					if (isWh2()) {
						k = (krand() % 40) + 5;
						break;
					}
					if (plr.currweaponattackstyle == 0)
						k = (krand() & 15) + 45;
					else
						k = (krand() & 15) + 40;
					break;
				case 9: // halberd
					if (isWh2()) {
						k = (krand() % 45) + 5;
						break;
					}

					if (plr.currweaponattackstyle == 0)
						k = (krand() & 15) + 25;
					else
						k = (krand() & 15) + 15;
					break;

				}

				k += plr.lvl;
				if (isWh2() && plr.weapon[plr.currweapon] == 3) {
					k <<= 1;
				}

				if (plr.vampiretime > 0) {
					if (plr.health <= plr.maxhealth)
						addhealth(plr, (krand() % 10) + 1);
				}
				if (plr.helmettime > 0)
					k <<= 1;
				if (plr.strongtime > 0) {
					k += k >> 1;

					switch (plr.currweapon) {
					case 0: // fist
						if (plr.currweaponframe == RFIST + 5)
							if (plr.currweaponanim == 5 && plr.currweapontics == 10)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 1: // knife
						if (plr.currweaponframe == KNIFEATTACK + 6)
							if (plr.currweaponanim == 8 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == KNIFEATTACK2 + 2)
							if (plr.currweaponanim == 5 || plr.currweaponanim == 9 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 2: // short sword
						if (plr.currweaponframe == GOBSWORDATTACK + 4 || plr.currweaponframe == ZSHORTATTACK + 7)
							if (plr.currweaponanim == 4 && plr.currweapontics == 10)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == GOBSWORDATTACK2 + 4 || plr.currweaponframe == ZSHORTATTACK2 + 4)
							if (plr.currweaponanim == 4 && plr.currweapontics == 10)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 3: // morning
						if (plr.currweaponframe == MORNINGSTAR + 5 || plr.currweaponframe == ZSTARATTACK + 7)
							if (plr.currweaponanim == 7 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == MORNINGATTACK2 + 3 || plr.currweaponframe == ZSTARATTACK2 + 3)
							if (plr.currweaponanim == 3 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 4: // sword
						if (plr.currweaponframe == SWORDATTACK + 7)
							if (plr.currweaponanim == 7 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == SWORDATTACK2 + 6)
							if (plr.currweaponanim == 6 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 5: // battleaxe
						if (plr.currweaponframe == BIGAXEATTACK + 7 || plr.currweaponframe == ZAXEATTACK + 12)
							if (plr.currweaponanim == 7 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == BIGAXEATTACK2 + 6 || plr.currweaponframe == ZAXEATTACK2 + 6)
							if (plr.currweaponanim == 6 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 6: // bow
						if (plr.currweaponframe == BOWWALK + 4)
							if (plr.currweaponanim == 4 && plr.currweapontics == 6)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == ZBOWATTACK + 4)
							if (plr.currweaponanim == 4 && plr.currweapontics == 6)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 7: // pike
						if (plr.currweaponframe == PIKEATTACK1 + 4 || plr.currweaponframe == ZPIKEATTACK + 4)
							if (plr.currweaponanim == 8 && plr.currweapontics == 10)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == PIKEATTACK2 + 4 || plr.currweaponframe == ZPIKEATTACK2 + 4)
							if (plr.currweaponanim == 4 && plr.currweapontics == 10)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 8: // two handed sword
						if (plr.currweaponframe == EXCALATTACK1 + 7 || plr.currweaponframe == ZTWOHANDATTACK + 12)
							if (plr.currweaponanim == 7 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == EXCALATTACK2 + 5 || plr.currweaponframe == ZTWOHANDATTACK2 + 5)
							if (plr.currweaponanim == 5 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 9: // halberd
						if (plr.currweaponframe == HALBERDATTACK1 + 3 || plr.currweaponframe == ZHALBERDATTACK + 4)
							if (plr.currweaponanim == 6 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == HALBERDATTACK2 + 3 || plr.currweaponframe == ZHALBERDATTACK2 + 3)
							if (plr.currweaponanim == 4 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						break;
					}
					sprite[pHitInfo.hitsprite].hitag -= (k << 1);
					if (isWh2() && plr.weapon[plr.currweapon] == 3 && plr.currweapon == 8
							&& sprite[pHitInfo.hitsprite].pal != 6) {
						if (sprite[pHitInfo.hitsprite].hitag <= 0) {
							sprite[pHitInfo.hitsprite].hitag = 1;
						}
						if (krand() % 100 > 50)
							medusa(plr, pHitInfo.hitsprite);
						break;
					}

					else if (plr.currweapon != 0) {

						// JSA GORE1 you have strong time
						if (krand() % 100 > 50) {
							if (sprite[pHitInfo.hitsprite].picnum == SKELETON
									|| sprite[pHitInfo.hitsprite].picnum == SKELETONATTACK
									|| sprite[pHitInfo.hitsprite].picnum == SKELETONDIE)
								spritesound(S_SKELHIT1 + (krand() % 2), &sprite[pHitInfo.hitsprite]);
						}

						// HERE
						switch (plr.currweapon) {
						case 0: // fist
							break;
						case 1: // knife
							if (plr.currweaponframe == KNIFEATTACK + 6)
								if (plr.currweaponanim == 8 && plr.currweapontics == 8)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == KNIFEATTACK2 + 2)
								if (plr.currweaponanim == 5 || plr.currweaponanim == 9 && plr.currweapontics == 8)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 2: // short sword
							if (plr.currweaponframe == GOBSWORDATTACK + 4
									|| plr.currweaponframe == ZSHORTATTACK + 7)
								if (plr.currweaponanim == 4 && plr.currweapontics == 10)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == GOBSWORDATTACK2 + 4
									|| plr.currweaponframe == ZSHORTATTACK + 4)
								if (plr.currweaponanim == 4 && plr.currweapontics == 10)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 3: // morning
							if (plr.currweaponframe == MORNINGSTAR + 5 || plr.currweaponframe == ZSTARATTACK + 7)
								if (plr.currweaponanim == 7 && plr.currweapontics == 12)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == MORNINGATTACK2 + 3
									|| plr.currweaponframe == ZSTARATTACK2 + 3)
								if (plr.currweaponanim == 3 && plr.currweapontics == 12)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 4: // sword
							if (plr.currweaponframe == SWORDATTACK + 7)
								if (plr.currweaponanim == 7 && plr.currweapontics == 8)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == SWORDATTACK2 + 6)
								if (plr.currweaponanim == 6 && plr.currweapontics == 8)
									break;
							chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity, pHitInfo.hitz,
									pHitInfo.hitsect, daang);
						case 5: // battleaxe
							if (plr.currweaponframe == BIGAXEATTACK + 7 || plr.currweaponframe == ZAXEATTACK + 12)
								if (plr.currweaponanim == 7 && plr.currweapontics == 12)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == BIGAXEATTACK2 + 6 || plr.currweaponframe == ZAXEATTACK2 + 6)
								if (plr.currweaponanim == 6 && plr.currweapontics == 12)
									break;
							chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity, pHitInfo.hitz,
									pHitInfo.hitsect, daang);
						case 6: // bow
							if (plr.currweaponframe == BOWWALK + 4)
								if (plr.currweaponanim == 4 && plr.currweapontics == 6)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == ZBOWATTACK + 4)
								if (plr.currweaponanim == 4 && plr.currweapontics == 6)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 7: // pike
							if (plr.currweaponframe == PIKEATTACK1 + 4 || plr.currweaponframe == ZPIKEATTACK + 4)
								if (plr.currweaponanim == 8 && plr.currweapontics == 10)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == PIKEATTACK2 + 4 || plr.currweaponframe == ZPIKEATTACK2 + 4)
								if (plr.currweaponanim == 4 && plr.currweapontics == 10)
									break;
							chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity, pHitInfo.hitz,
									pHitInfo.hitsect, daang);
						case 8: // two handed sword
							if (plr.currweaponframe == EXCALATTACK1 + 7
									|| plr.currweaponframe == ZTWOHANDATTACK + 12)
								if (plr.currweaponanim == 7 && plr.currweapontics == 8)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == EXCALATTACK2 + 5
									|| plr.currweaponframe == ZTWOHANDATTACK2 + 5)
								if (plr.currweaponanim == 5 && plr.currweapontics == 8)
									break;
							chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity, pHitInfo.hitz,
									pHitInfo.hitsect, daang);
						case 9: // halberd
							if (plr.currweaponframe == HALBERDATTACK1 + 3
									|| plr.currweaponframe == ZHALBERDATTACK + 4)
								if (plr.currweaponanim == 6 && plr.currweapontics == 12)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == HALBERDATTACK2 + 3
									|| plr.currweaponframe == ZHALBERDATTACK2 + 3)
								if (plr.currweaponanim == 4 && plr.currweapontics == 12)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						}
					}

				} else {
					switch (plr.currweapon) {
					case 0: // fist
						if (plr.currweaponframe == RFIST + 5)
							if (plr.currweaponanim == 5 && plr.currweapontics == 10)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 1: // knife
						if (plr.currweaponframe == KNIFEATTACK + 6)
							if (plr.currweaponanim == 8 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == KNIFEATTACK2 + 2)
							if (plr.currweaponanim == 5 || plr.currweaponanim == 9 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 2: // SHORT SWORD
						if (plr.currweaponframe == GOBSWORDATTACK + 4 || plr.currweaponframe == ZSHORTATTACK + 7)
							if (plr.currweaponanim == 4 && plr.currweapontics == 10)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == GOBSWORDATTACK2 + 4 || plr.currweaponframe == ZSHORTATTACK2 + 4)
							if (plr.currweaponanim == 4 && plr.currweapontics == 10)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 3: // morning
						if (plr.currweaponframe == MORNINGSTAR + 5 || plr.currweaponframe == ZSTARATTACK + 7)
							if (plr.currweaponanim == 7 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == MORNINGATTACK2 + 3 || plr.currweaponframe == ZSTARATTACK2 + 3)
							if (plr.currweaponanim == 3 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 4: // sword
						if (plr.currweaponframe == SWORDATTACK + 7)
							if (plr.currweaponanim == 7 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == SWORDATTACK2 + 6)
							if (plr.currweaponanim == 6 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 5: // battleaxe
						if (plr.currweaponframe == BIGAXEATTACK + 7 || plr.currweaponframe == ZAXEATTACK + 12)
							if (plr.currweaponanim == 7 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == BIGAXEATTACK2 + 6 || plr.currweaponframe == ZAXEATTACK2 + 6)
							if (plr.currweaponanim == 6 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 6: // bow
						if (plr.currweaponframe == BOWWALK + 4)
							if (plr.currweaponanim == 4 && plr.currweapontics == 6)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == ZBOWATTACK + 4)
							if (plr.currweaponanim == 4 && plr.currweapontics == 6)
								swingdacrunch(plr, plr.currweapon);
						break;

					case 7: // pike
						if (plr.currweaponframe == PIKEATTACK1 + 4 || plr.currweaponframe == ZPIKEATTACK + 4)
							if (plr.currweaponanim == 8 && plr.currweapontics == 10)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == PIKEATTACK2 + 4 || plr.currweaponframe == ZPIKEATTACK2 + 4)
							if (plr.currweaponanim == 4 && plr.currweapontics == 10)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 8: // two handed sword
						if (plr.currweaponframe == EXCALATTACK1 + 7 || plr.currweaponframe == ZTWOHANDATTACK + 12)
							if (plr.currweaponanim == 7 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == EXCALATTACK2 + 5 || plr.currweaponframe == ZTWOHANDATTACK2 + 5)
							if (plr.currweaponanim == 5 && plr.currweapontics == 8)
								swingdacrunch(plr, plr.currweapon);
						break;
					case 9: // halberd
						if (plr.currweaponframe == HALBERDATTACK1 + 3 || plr.currweaponframe == ZHALBERDATTACK + 4)
							if (plr.currweaponanim == 6 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						if (plr.currweaponframe == HALBERDATTACK2 + 3 || plr.currweaponframe == ZHALBERDATTACK2 + 3)
							if (plr.currweaponanim == 4 && plr.currweapontics == 12)
								swingdacrunch(plr, plr.currweapon);
						break;
					}
					sprite[pHitInfo.hitsprite].hitag -= k;

					if (isWh2() && plr.weapon[plr.currweapon] == 3 && plr.currweapon == 8
							&& sprite[pHitInfo.hitsprite].pal != 6) {
						if (sprite[pHitInfo.hitsprite].hitag <= 0) {
							sprite[pHitInfo.hitsprite].hitag = 1;
						}
						if (krand() % 100 > 75)
							medusa(plr, pHitInfo.hitsprite);
						break;
					}

					if (plr.currweapon != 0) {
						// JSA GORE normal
						if (krand() % 100 > 50) {
							if (sprite[pHitInfo.hitsprite].picnum == SKELETON
									|| sprite[pHitInfo.hitsprite].picnum == SKELETONATTACK
									|| sprite[pHitInfo.hitsprite].picnum == SKELETONDIE)
								spritesound(S_SKELHIT1 + (krand() % 2), &sprite[pHitInfo.hitsprite]);
						}
						// HERE
						switch (plr.currweapon) {
						case 0: // fist
							break;
						case 1: // knife
							if (plr.currweaponframe == KNIFEATTACK + 6)
								if (plr.currweaponanim == 8 && plr.currweapontics == 8)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == KNIFEATTACK2 + 2)
								if (plr.currweaponanim == 5 || plr.currweaponanim == 9 && plr.currweapontics == 8)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 2: // short sword
							if (plr.currweaponframe == GOBSWORDATTACK + 4
									|| plr.currweaponframe == ZSHORTATTACK + 7)
								if (plr.currweaponanim == 4 && plr.currweapontics == 10)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == GOBSWORDATTACK2 + 4
									|| plr.currweaponframe == ZSHORTATTACK2 + 4)
								if (plr.currweaponanim == 4 && plr.currweapontics == 10)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 3: // morning
							if (plr.currweaponframe == MORNINGSTAR + 5 || plr.currweaponframe == ZSTARATTACK + 7)
								if (plr.currweaponanim == 7 && plr.currweapontics == 12)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == MORNINGATTACK2 + 3
									|| plr.currweaponframe == ZSTARATTACK2 + 3)
								if (plr.currweaponanim == 3 && plr.currweapontics == 12)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 4: // sword
							if (plr.currweaponframe == SWORDATTACK + 7)
								if (plr.currweaponanim == 7 && plr.currweapontics == 8)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == SWORDATTACK2 + 6)
								if (plr.currweaponanim == 6 && plr.currweapontics == 8)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 5: // battleaxe
							if (plr.currweaponframe == BIGAXEATTACK + 7 || plr.currweaponframe == ZAXEATTACK + 12)
								if (plr.currweaponanim == 7 && plr.currweapontics == 12)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == BIGAXEATTACK2 + 6 || plr.currweaponframe == ZAXEATTACK2 + 6)
								if (plr.currweaponanim == 6 && plr.currweapontics == 12)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 6: // bow
							if (plr.currweaponframe == BOWWALK + 4)
								if (plr.currweaponanim == 4 && plr.currweapontics == 6)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == ZBOWATTACK + 4)
								if (plr.currweaponanim == 4 && plr.currweapontics == 6)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 7: // pike
							if (plr.currweaponframe == PIKEATTACK1 + 4 || plr.currweaponframe == ZPIKEATTACK + 4)
								if (plr.currweaponanim == 8 && plr.currweapontics == 10)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == PIKEATTACK2 + 4 || plr.currweaponframe == ZPIKEATTACK2 + 4)
								if (plr.currweaponanim == 4 && plr.currweapontics == 10)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 8: // two handed sword
							if (plr.currweaponframe == EXCALATTACK1 + 7
									|| plr.currweaponframe == ZTWOHANDATTACK + 12)
								if (plr.currweaponanim == 7 && plr.currweapontics == 8)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == EXCALATTACK2 + 5
									|| plr.currweaponframe == ZTWOHANDATTACK2 + 5)
								if (plr.currweaponanim == 5 && plr.currweapontics == 8)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						case 9: // halberd
							if (plr.currweaponframe == HALBERDATTACK1 + 3
									|| plr.currweaponframe == ZHALBERDATTACK + 4)
								if (plr.currweaponanim == 6 && plr.currweapontics == 12)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							if (plr.currweaponframe == HALBERDATTACK2 + 3
									|| plr.currweaponframe == ZHALBERDATTACK2 + 3)
								if (plr.currweaponanim == 4 && plr.currweapontics == 12)
									chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity,
											pHitInfo.hitz, pHitInfo.hitsect, daang);
							break;
						}
					}
				}

				if (netgame) {
					break;
				}

				if (sprite[pHitInfo.hitsprite].hitag <= 0) {
					if (plr.selectedgun > 1) {
						// JSA GORE on death ?
						// RAF ans:death
						chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity, pHitInfo.hitz,
								pHitInfo.hitsect, daang);
						if (sprite[pHitInfo.hitsprite].picnum == SKELETON
								|| sprite[pHitInfo.hitsprite].picnum == SKELETONATTACK
								|| sprite[pHitInfo.hitsprite].picnum == SKELETONDIE)
							spritesound(S_SKELHIT1 + (krand() % 2), &sprite[pHitInfo.hitsprite]);
					}
					newstatus(pHitInfo.hitsprite, DIE);
				}
				sprite[pHitInfo.hitsprite].ang = (short) (plr.ang + ((krand() & 32) - 64));
				if (sprite[pHitInfo.hitsprite].hitag > 0)
					newstatus(pHitInfo.hitsprite, PAIN);
				break;
			} // switch enemytype

			switch (sprite[pHitInfo.hitsprite].detail) {
			case GRONTYPE:
			case KOBOLDTYPE:
			case DRAGONTYPE:
			case DEVILTYPE:
			case FREDTYPE:
			case SKELETONTYPE:
			case GOBLINTYPE:
			case IMPTYPE:
			case MINOTAURTYPE:
			case SKULLYTYPE:
			case SPIDERTYPE:
			case FATWITCHTYPE:
			case JUDYTYPE:
			case NEWGUYTYPE:
			case GONZOTYPE:
			case KURTTYPE:
				if (sprite[pHitInfo.hitsprite].pal == 6) {
					// JSA_NEW
					SND_Sound(S_SOCK1 + (krand() % 4));
					playsound(S_FREEZEDIE, pHitInfo.hitx, pHitInfo.hity);
					for (k = 0; k < 32; k++)
						icecubes(pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity, pHitInfo.hitz,
								pHitInfo.hitsprite);
					addscore(&plr, 100);
					deletesprite((short) pHitInfo.hitsprite);
				}
				break;
			} // switch frozen

			switch (sprite[pHitInfo.hitsprite].picnum) {
			case STAINGLASS1:
			case STAINGLASS2:
			case STAINGLASS3:
			case STAINGLASS4:
			case STAINGLASS5:
			case STAINGLASS6:
			case STAINGLASS7:
			case STAINGLASS8:
			case STAINGLASS9:
				if (!isWh2())
					break;
			case BARREL:
			case VASEA:
			case VASEB:
			case VASEC:

				newstatus(pHitInfo.hitsprite, BROKENVASE);
				break;
			} // switch
		} // if weapondist

		if (!madeahit) {
			plr.ammo[plr.currweapon]++;
			madeahit = true;
		}
		break;
	case 1: //bow's arrow
		daz2 = -mulscale16(plr.horizon.horiz.asq16(), 2000);

		hitscan(plr.x, plr.y, plr.z, plr.sector, // Start position
				sintable[(daang + 2560) & 2047], // X vector of 3D ang
				sintable[(daang + 2048) & 2047], // Y vector of 3D ang
				daz2, // Z vector of 3D ang
				pHitInfo, CLIPMASK1);

		if (pHitInfo.hitwall > 0 && pHitInfo.hitsprite < 0) { // XXX WH2 sector lotag < 6 || > 8
			if (isWh2()) {
				arrowcnt = (arrowcnt + 1) % ARROWCOUNTLIMIT;
				if (arrowsprite[arrowcnt] != -1) {
					deletesprite((short) arrowsprite[arrowcnt]);
					arrowsprite[arrowcnt] = -1;
				}
			}

			Neartag ntag;
			neartag(pHitInfo.hitx, pHitInfo.hity, pHitInfo.hitz, (short) pHitInfo.hitsect, (short) daang,
					ntag, 1024, 3);

			if (ntag.tagsector < 0) {
				j = insertsprite(pHitInfo.hitsect, (short) 0);
				sprite[j].x = pHitInfo.hitx;
				sprite[j].y = pHitInfo.hity;
				sprite[j].z = pHitInfo.hitz + (8 << 8);
				sprite[j].cstat = 17;// was16
				sprite[j].picnum = WALLARROW;
				sprite[j].shade = 0;
				sprite[j].pal = 0;
				sprite[j].xrepeat = 16;
				sprite[j].yrepeat = 48;
				sprite[j].ang = (short) (((daang) - 512 + (krand() & 128 - 64)) & 2047);
				sprite[j].xvel = 0;
				sprite[j].yvel = 0;
				sprite[j].zvel = 0;
				sprite[j].owner = sprite[plr.spritenum].owner;
				sprite[j].lotag = 32;
				sprite[j].hitag = 0;
				spritesound(S_ARROWHIT, &sprite[j]);

				if (isWh2() && plr.weapon[6] == 3 && plr.currweapon == 6) {
					j = insertsprite(pHitInfo.hitsect, FIRECHUNK);
					sprite[j].x = pHitInfo.hitx;
					sprite[j].y = pHitInfo.hity;
					sprite[j].z = pHitInfo.hitz + (14 << 8);
					sprite[j].cstat = 0;
					sprite[j].picnum = ARROWFLAME;
					sprite[j].shade = 0;
					sprite[j].pal = 0;
					sprite[j].xrepeat = 64;
					sprite[j].yrepeat = 64;
					sprite[j].ang = 0;
					sprite[j].xvel = 0;
					sprite[j].yvel = 0;
					sprite[j].zvel = 0;
					sprite[j].owner = 0;
					sprite[j].lotag = 1200;
					sprite[j].hitag = 0;
				}
			}

			if (netgame) {
//					netshootgun(-1,5);
			}
		}
		if (pHitInfo.hitwall > 0 && pHitInfo.hitsprite > 0) {
			j = insertsprite(pHitInfo.hitsect, FX);
			sprite[j].x = pHitInfo.hitx;
			sprite[j].y = pHitInfo.hity;
			sprite[j].z = pHitInfo.hitz + (8 << 8);
			sprite[j].cstat = 2;
			sprite[j].picnum = PLASMA;
			sprite[j].shade = -32;
			sprite[j].pal = 0;
			sprite[j].xrepeat = 32;
			sprite[j].yrepeat = 32;
			sprite[j].ang = (short) daang;
			sprite[j].xvel = 0;
			sprite[j].yvel = 0;
			sprite[j].zvel = 0;
			sprite[j].owner = sprite[plr.spritenum].owner;
			sprite[j].lotag = 32;
			sprite[j].hitag = 0;
			movesprite((short) j, ((sintable[(sprite[j].ang + 512) & 2047]) * TICSPERFRAME) << 3,
					((sintable[sprite[j].ang & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
		}
		if ((pHitInfo.hitsprite >= 0) && (sprite[pHitInfo.hitsprite].statnum < MAXSTATUS)) {
			switch (sprite[pHitInfo.hitsprite].detail) {
			case KURTTYPE:
			case KATIETYPE:
			case NEWGUYTYPE:
			case GONZOTYPE:
			case GRONTYPE:
			case KOBOLDTYPE:
			case DRAGONTYPE:
			case DEMONTYPE:
			case DEVILTYPE:
			case FREDTYPE:
			case SKELETONTYPE:
			case GOBLINTYPE:
			case IMPTYPE:
			case MINOTAURTYPE:
			case SPIDERTYPE:
			case SKULLYTYPE:
			case FATWITCHTYPE:
			case FISHTYPE:
			case RATTYPE:
			case WILLOWTYPE:
			case GUARDIANTYPE:
			case JUDYTYPE:
				if (netgame) {
//						netshootgun(pHitInfo.hitsprite,currweapon);
					break;
				}
				if (isWh2())
					sprite[pHitInfo.hitsprite].hitag -= (krand() & 30) + 15;
				else
					sprite[pHitInfo.hitsprite].hitag -= (krand() & 15) + 15;

				if (sprite[pHitInfo.hitsprite].hitag <= 0) {
					newstatus(pHitInfo.hitsprite, DIE);
					if (sprite[pHitInfo.hitsprite].picnum == RAT)
						chunksofmeat(plr, pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity, pHitInfo.hitz,
								pHitInfo.hitsect, daang);
				} else {
					sprite[pHitInfo.hitsprite].ang = (short) (getangle(plr.x - sprite[pHitInfo.hitsprite].x,
							plr.y - sprite[pHitInfo.hitsprite].y) & 2047);
					newstatus(pHitInfo.hitsprite, PAIN);
				}
				break;
			}

			switch (sprite[pHitInfo.hitsprite].detail) {
			// SHATTER FROZEN CRITTER
			case GRONTYPE:
			case KOBOLDTYPE:
			case DRAGONTYPE:
			case DEVILTYPE:
			case FREDTYPE:
			case SKELETONTYPE:
			case GOBLINTYPE:
			case IMPTYPE:
			case MINOTAURTYPE:
			case SKULLYTYPE:
			case SPIDERTYPE:
			case FATWITCHTYPE:
			case JUDYTYPE:
			case NEWGUYTYPE:
			case GONZOTYPE:
			case KURTTYPE:
				if (sprite[pHitInfo.hitsprite].pal == 6) {
					// JSA_NEW
					SND_Sound(S_SOCK1 + (krand() % 4));
					playsound(S_FREEZEDIE, pHitInfo.hitx, pHitInfo.hity);
					for (k = 0; k < 32; k++)
						icecubes(pHitInfo.hitsprite, pHitInfo.hitx, pHitInfo.hity, pHitInfo.hitz,
								pHitInfo.hitsprite);
					deletesprite((short) pHitInfo.hitsprite);
				}
			} // switch frozen

			switch (sprite[pHitInfo.hitsprite].picnum) {
			case STAINGLASS1:
			case STAINGLASS2:
			case STAINGLASS3:
			case STAINGLASS4:
			case STAINGLASS5:
			case STAINGLASS6:
			case STAINGLASS7:
			case STAINGLASS8:
			case STAINGLASS9:
				if (!isWh2())
					break;

			case BARREL:
			case VASEA:
			case VASEB:
			case VASEC:
				newstatus(pHitInfo.hitsprite, BROKENVASE);
				break;
			} // switch
		}
		break;

	case 6: // MEDUSA
		for (i = 0; i < MAXSPRITES; i++) {
			// cansee
			if (i != plr.spritenum) {
				switch (sprite[i].detail) {
				case FREDTYPE:
				case KOBOLDTYPE:
				case GOBLINTYPE:
				case IMPTYPE:
				case MINOTAURTYPE:
				case SPIDERTYPE:
				case SKELETONTYPE:
				case GRONTYPE:
				case GONZOTYPE:
				case KURTTYPE:
				case NEWGUYTYPE:
					if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[i].x, sprite[i].y,
							sprite[i].z - (tileHeight(sprite[i].picnum) << 7), sprite[i].sectnum)) {
						// distance check
						if (checkmedusadist(i, plr.x, plr.y, plr.z, plr.lvl))
							medusa(plr, i);
					}
					break;
				}
			}
		}
		break;
	case 7: // KNOCKSPELL
	{
		daz2 = -mulscale16(plr.horizon.horiz.asq16(), 2000);

		Neartag ntag;
		hitscan(plr.x, plr.y, plr.z, plr.sector, // Start position
			sintable[(daang + 2560) & 2047], // X vector of 3D ang
			sintable[(daang + 2048) & 2047], // Y vector of 3D ang
			daz2, // Z vector of 3D ang
			pHitInfo, CLIPMASK1);

		if (pHitInfo.hitsect < 0 && pHitInfo.hitsprite < 0 || pHitInfo.hitwall >= 0) {

			neartag(pHitInfo.hitx, pHitInfo.hity, pHitInfo.hitz, (short)pHitInfo.hitsect, (short)daang,	ntag, 1024, 3);

			if (ntag.tagsector >= 0) {
				if (sector[ntag.tagsector].lotag >= 60 && sector[ntag.tagsector].lotag <= 69) {
					sector[ntag.tagsector].lotag = 6;
					sector[ntag.tagsector].hitag = 0;
				}
				if (sector[ntag.tagsector].lotag >= 70 && sector[ntag.tagsector].lotag <= 79) {
					sector[ntag.tagsector].lotag = 7;
					sector[ntag.tagsector].hitag = 0;
				}
				operatesector(plr, ntag.tagsector);
			}

		}
		break;
	}
	case 10: // throw a pike axe
		if (plr.currweaponframe == PIKEATTACK1 + 4 || plr.currweaponframe == ZPIKEATTACK + 4) {
			if (plr.currweaponanim == 8 && plr.currweapontics == 10) {
				if (netgame) {
					// netshootgun(-1,15);
				}

				if (isWh2()) {
					throwpikecnt = (throwpikecnt + 1) % THROWPIKELIMIT;
					if (throwpikesprite[throwpikecnt] != -1) {
						deletesprite(throwpikesprite[throwpikecnt]);
						throwpikesprite[throwpikecnt] = -1;
					}

					if (plr.weapon[plr.currweapon] == 3) {
						j = insertsprite(plr.sector, MISSILE);
						throwpikesprite[throwpikecnt] = j;
						sprite[j].x = plr.x;
						sprite[j].y = plr.y;
						sprite[j].z = plr.z + (24 << 8);
						sprite[j].cstat = 21;
						sprite[j].picnum = THROWPIKE;
						sprite[j].ang = (short) (((daang + 2048 + 96) - 512) & 2047);
						sprite[j].xrepeat = 24;
						sprite[j].yrepeat = 24;
						sprite[j].clipdist = 32;
						sprite[j].extra = (short) daang;
						sprite[j].shade = -15;
						sprite[j].xvel = (short) ((krand() & 256) - 128);
						sprite[j].yvel = (short) ((krand() & 256) - 128);
						if (shootgunzvel != 0) {
							sprite[j].zvel = (short) shootgunzvel;
							shootgunzvel = 0;
						} else {
							sprite[j].zvel = plr.horizon.horiz.asq16() >> 12;
						}
						sprite[j].owner = sprite[plr.spritenum].owner;
						sprite[j].lotag = 1024;
						sprite[j].hitag = 0;
						sprite[j].pal = 0;
						movesprite((short) j, ((sintable[(sprite[j].extra + 512) & 2047]) * TICSPERFRAME) << 3,
								((sintable[sprite[j].extra & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
						setsprite((short) j, sprite[j].x, sprite[j].y, sprite[j].z);
					} else {
						j = insertsprite(plr.sector, MISSILE);
						throwpikesprite[throwpikecnt] = j;
						sprite[j].x = plr.x;
						sprite[j].y = plr.y;
						sprite[j].z = plr.z + (24 << 8);
						sprite[j].cstat = 21;
						sprite[j].picnum = THROWPIKE;
						sprite[j].ang = (short) ((((int) plr.ang + 2048 + 96) - 512) & 2047);
						sprite[j].xrepeat = 24;
						sprite[j].yrepeat = 24;
						sprite[j].clipdist = 32;
						sprite[j].extra = (short) plr.ang;
						sprite[j].shade = -15;
						sprite[j].xvel = (short) ((krand() & 256) - 128);
						sprite[j].yvel = (short) ((krand() & 256) - 128);
						sprite[j].zvel = plr.horizon.horiz.asq16() >> 12;
						sprite[j].owner = sprite[plr.spritenum].owner;
						sprite[j].lotag = 1024;
						sprite[j].hitag = 0;
						sprite[j].pal = 0;
					}
				} else {
					j = insertsprite(plr.sector, MISSILE);

					sprite[j].x = plr.x;
					sprite[j].y = plr.y;
					sprite[j].z = plr.z + (16 << 8);

					// sprite[j].cstat=17;
					sprite[j].cstat = 21;

					sprite[j].picnum = THROWPIKE;
					sprite[j].ang = (short) BClampAngle((plr.ang + 96) - 512);
					sprite[j].xrepeat = 24;
					sprite[j].yrepeat = 24;
					sprite[j].clipdist = 24;

					sprite[j].extra = (short) plr.ang;
					sprite[j].shade = -15;
					sprite[j].xvel = (short) ((krand() & 256) - 128);
					sprite[j].yvel = (short) ((krand() & 256) - 128);
					// sprite[j].zvel=((krand()&256)-128);
					sprite[j].zvel = plr.horizon.horiz.asq16() >> 12;
					sprite[j].owner = sprite[plr.spritenum].owner;
					sprite[j].lotag = 1024;
					sprite[j].hitag = 0;
					sprite[j].pal = 0;
				}
			}
		}

		if (plr.currweaponframe == PIKEATTACK2 + 4) {
			if (plr.currweaponanim == 4 && plr.currweapontics == 10) {

				if (isWh2()) {
					throwpikecnt = (throwpikecnt + 1) % THROWPIKELIMIT;
					if (throwpikesprite[throwpikecnt] != -1) {
						deletesprite((short) throwpikesprite[throwpikecnt]);
						throwpikesprite[throwpikecnt] = -1;
					}

					if (plr.weapon[plr.currweapon] == 3) {

						j = insertsprite(plr.sector, MISSILE);
						throwpikesprite[throwpikecnt] = j;
						sprite[j].x = plr.x;
						sprite[j].y = plr.y;
						sprite[j].z = plr.z + (24 << 8);
						sprite[j].cstat = 21;
						sprite[j].picnum = THROWPIKE;
						sprite[j].ang = (short) daang;

						sprite[j].xrepeat = 24;
						sprite[j].yrepeat = 24;
						sprite[j].clipdist = 32;
						sprite[j].extra = (short) daang;

						sprite[j].shade = -15;
						sprite[j].xvel = (short) ((krand() & 256) - 128);
						sprite[j].yvel = (short) ((krand() & 256) - 128);
						sprite[j].zvel = plr.horizon.horiz.asq16() >> 12;
						sprite[j].owner = sprite[plr.spritenum].owner;
						sprite[j].lotag = 1024;
						sprite[j].hitag = 0;
						sprite[j].pal = 0;
						movesprite((short) j, ((sintable[(sprite[j].extra + 512) & 2047]) * TICSPERFRAME) << 3,
								((sintable[sprite[j].extra & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
						setsprite((short) j, sprite[j].x, sprite[j].y, sprite[j].z);
					} else {
						j = insertsprite(plr.sector, MISSILE);
						throwpikesprite[throwpikecnt] = j;
						sprite[j].x = plr.x;
						sprite[j].y = plr.y;
						sprite[j].z = plr.z + (24 << 8);
						sprite[j].cstat = 21;
						sprite[j].picnum = THROWPIKE;
						sprite[j].ang = (short) ((((int) plr.ang + 2048 + 96) - 512) & 2047);
						sprite[j].xrepeat = 24;
						sprite[j].yrepeat = 24;
						sprite[j].clipdist = 32;
						sprite[j].extra = (short) plr.ang;
						sprite[j].shade = -15;
						sprite[j].xvel = (short) ((krand() & 256) - 128);
						sprite[j].yvel = (short) ((krand() & 256) - 128);
						sprite[j].zvel = plr.horizon.horiz.asq16() >> 12;
						sprite[j].owner = sprite[plr.spritenum].owner;
						sprite[j].lotag = 1024;
						sprite[j].hitag = 0;
						sprite[j].pal = 0;
					}
				} else {
					j = insertsprite(plr.sector, MISSILE);

					sprite[j].x = plr.x;
					sprite[j].y = plr.y;
					sprite[j].z = plr.z;

					sprite[j].cstat = 21;

					sprite[j].picnum = THROWPIKE;
					sprite[j].ang = (short) BClampAngle((plr.ang) - 512);
					sprite[j].xrepeat = 24;
					sprite[j].yrepeat = 24;
					sprite[j].clipdist = 24;

					sprite[j].extra = (short) plr.ang;
					sprite[j].shade = -15;
					sprite[j].xvel = (short) ((krand() & 256) - 128);
					sprite[j].yvel = (short) ((krand() & 256) - 128);
					sprite[j].zvel = (short) ((krand() & 256) - 128);
					sprite[j].owner = sprite[plr.spritenum].owner;
					sprite[j].lotag = 1024;
					sprite[j].hitag = 0;
					sprite[j].pal = 0;
				}

			}
		}
		break;
	case 2: // parabolic trajectory

		if (netgame) {
//						netshootgun(-1,12);
		}

		j = insertsprite(plr.sector, MISSILE);
		sprite[j].x = plr.x;
		sprite[j].y = plr.y;
		sprite[j].z = plr.z + (8 << 8) + ((krand() & 10) << 8);
		sprite[j].cstat = 0;
		sprite[j].picnum = PLASMA;
		sprite[j].shade = -32;
		sprite[j].pal = 0;
		sprite[j].xrepeat = 16;
		sprite[j].yrepeat = 16;
		sprite[j].ang = (short) daang;
		sprite[j].xvel = (short) (sintable[(daang + 2560) & 2047] >> 5);
		sprite[j].yvel = (short) (sintable[(daang) & 2047] >> 5);

		if (shootgunzvel != 0) {
			sprite[j].zvel = (short) shootgunzvel;
			shootgunzvel = 0;
		} else {
			sprite[j].zvel = plr.horizon.horiz.asq16() >> 12;
		}

		sprite[j].owner = sprite[plr.spritenum].owner;
		sprite[j].lotag = 256;
		sprite[j].hitag = 0;
		sprite[j].clipdist = 48;

		movesprite((short) j, ((sintable[(sprite[j].ang + 512) & 2047]) * TICSPERFRAME) << 3,
				((sintable[sprite[j].ang & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
		setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);

		break;
	case 3:

		if (netgame) {
//						netshootgun(-1,13);
		}

		j = insertsprite(plr.sector, MISSILE);
		sprite[j].x = plr.x;
		sprite[j].y = plr.y;
		sprite[j].z = plr.z + (8 << 8);
		sprite[j].cstat = 0; // Hitscan does not hit other bullets
		sprite[j].picnum = MONSTERBALL;
		sprite[j].shade = -32;
		sprite[j].pal = 0;
		sprite[j].xrepeat = 64;
		sprite[j].yrepeat = 64;
		sprite[j].ang = (short) plr.ang;
		sprite[j].xvel = (short) (sintable[(daang + 2560) & 2047] >> 7);
		sprite[j].yvel = (short) (sintable[(daang) & 2047] >> 7);

		if (shootgunzvel != 0) {
			sprite[j].zvel = (short) shootgunzvel;
			shootgunzvel = 0;
		} else {
			sprite[j].zvel = plr.horizon.horiz.asq16() >> 12;
		}

		sprite[j].owner = sprite[plr.spritenum].owner;
		sprite[j].lotag = 256;
		sprite[j].hitag = 0;
		sprite[j].clipdist = 64;

		// dax=(sintable[(sprite[j].ang+512)&2047]>>6);
		// day=(sintable[sprite[j].ang]>>6);

		movesprite((short) j, ((sintable[(sprite[j].ang + 512) & 2047]) * TICSPERFRAME) << 3,
				((sintable[sprite[j].ang & 2047]) * TICSPERFRAME) << 3, 0, 4 << 8, 4 << 8, 0);
		setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);

		break;
	case 4:

		if (netgame) {
//						netshootgun(-1,14);
		}

		for (j = 0; j < MAXSPRITES; j++) {
			switch (sprite[j].detail) {
			case DEMONTYPE:
			case NEWGUYTYPE:
			case KURTTYPE:
			case GONZOTYPE:
			case IMPTYPE:
				if (!isWh2())
					break;

			case SPIDERTYPE:
			case KOBOLDTYPE:
			case DEVILTYPE:
			case GOBLINTYPE:
			case MINOTAURTYPE:
			case SKELETONTYPE:
			case GRONTYPE:
			case DRAGONTYPE:
			case GUARDIANTYPE:
			case FATWITCHTYPE:
			case SKULLYTYPE:
			case JUDYTYPE:
			case WILLOWTYPE:
				if (cansee(plr.x, plr.y, plr.z, plr.sector, sprite[j].x, sprite[j].y,
						sprite[j].z - (tileHeight(sprite[j].picnum) << 7), sprite[j].sectnum))
					if ((isWh2() && sprite[j].owner != sprite[plr.spritenum].owner)
							|| checkmedusadist(j, plr.x, plr.y, plr.z, 12))
						nukespell(plr, j);
				break;
			}
		}
		break;
	}
}

boolean checkweapondist(int i, int x, int y, int z, int guntype) {
	int length = 1024;

	if (guntype != 0) {
		switch (guntype) {
		case 1:
			length = 1024;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			length = 1536;
			break;
		case 6:
			length = 2048;
			break;
		case 7:
			length = 1024;
			break;
		case 8:
		case 9:
			length = 2048;
			break;
		}
	}

	if (i >= 0 && (abs(x - sprite[i].x) + abs(y - sprite[i].y) < length)
			&& (abs((z >> 8) - ((sprite[i].z >> 8) - (tileHeight(sprite[i].picnum) >> 1))) <= (length >> 3)))
		return true;
	else
		return false;
}

void swingdapunch(PLAYER& plr, int daweapon) {
	switch (daweapon) {
	case 0:// hands
		SND_Sound(S_SOCK4);
		SND_Sound(S_PLRPAIN1 + (krand() % 2));
		addhealth(plr, -1);
		startredflash(10);
		break;
	case 1: // knife
	case 2: // mace
	case 4: // sword
		SND_Sound(S_WALLHIT1);
		break;
	case 3: // arrow
		break;
	case 5:
	case 6:
	case 7:
	case 8:
		SND_Sound(S_WALLHIT1);
		break;
	}
}

void swingdaweapon(PLAYER& plr) {
	float daang = plr.ang;

	if (plr.currweaponframe == BOWWALK + 5 && plr.ammo[6] > 0) {
		plr.currweaponfired = 5;
		plr.currweaponanim = 0;
	}
	if (plr.currweaponframe == BOWWALK + 5 && plr.ammo[6] <= 0) {
		plr.currweaponfired = 0;
		plr.currweaponanim = 0;
		return;
	}

	if (!isWh2()) {
		if (plr.currweaponframe == PIKEATTACK1 + 4
				// || plr.currweaponframe == PIKEATTACK2+4
				&& plr.weapon[7] == 2 && plr.ammo[7] > 0) {
			shootgun(plr, daang, 10);
			spritesound(S_THROWPIKE, &sprite[plr.spritenum]);
			plr.hasshot = 1;
			return;
		}
	} else {
		if (plr.currweaponframe == BOWWALK + 5 && plr.ammo[6] > 0) {
			plr.currweaponfired = 5;
			plr.currweaponanim = 0;
		} else if (plr.currweaponframe == ZBOWATTACK + 4 && plr.ammo[6] > 0) {
			plr.currweaponfired = 5;
			plr.currweaponanim = 0;
		}

		if (plr.currweaponframe == BOWWALK + 5 && plr.ammo[6] <= 0) {
			plr.currweaponfired = 0;
			plr.currweaponanim = 0;
			return;
		} else if (plr.currweaponframe == ZBOWATTACK + 4 && plr.ammo[6] <= 0) {
			plr.currweaponfired = 0;
			plr.currweaponanim = 0;
			return;
		}

		if (plr.currweaponframe == PIKEATTACK1 + 4 && plr.weapon[7] == 2 && plr.ammo[7] > 0) {
			shootgun(plr, daang, 10);
			spritesound(S_GENTHROW, &sprite[plr.spritenum]);
			plr.hasshot = 1;
			return;
		} else if (plr.currweaponframe == ZPIKEATTACK + 4 && plr.weapon[7] == 3 && plr.ammo[7] > 0) {
			lockon(plr, 3, 10);
			spritesound(S_GENTHROW, &sprite[plr.spritenum]);
			plr.hasshot = 1;
			return;
		}
	}

	switch (plr.selectedgun) {
	case 0: // fist & close combat weapons
		shootgun(plr, daang, 0);
		plr.hasshot = 1;
		break;
	case 1: // knife
		shootgun(plr, daang, 0);
		plr.hasshot = 1;
		break;
	case 2: // shortsword
		shootgun(plr, daang, 0);
		plr.hasshot = 1;
		break;
	case 3: // morningstar
		shootgun(plr, daang, 0);
		if (isWh2() && plr.weapon[plr.selectedgun] == 3) {
			lockon(plr, 1, 3);
		}

		plr.hasshot = 1;
		break;
	case 4: // sword
		shootgun(plr, daang, 0);
		plr.hasshot = 1;
		break;
	case 5: // battleaxe
		if (isWh2() && soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, 0, CHAN_ENCHANTED) != 0)
				SND_Sound(S_ENERGYSWING);

		shootgun(plr, daang, 0);
		plr.hasshot = 1;
		break;
	case 6: // bow
		if (isWh2() && soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, 0, CHAN_ENCHANTED) != 0)
		{
			SND_Sound(S_FIREBALL);
			SND_Sound(S_PLRWEAPON3);
		}
		shootgun(plr, daang, 1);
		plr.hasshot = 1;
		break;
	case 7: // pike
		shootgun(plr, daang, 0);
		plr.hasshot = 1;
		break;
	case 8: // two handed
		shootgun(plr, daang, 0);
		plr.hasshot = 1;
		break;
	case 9: // halberd
		shootgun(plr, daang, 0);
		plr.hasshot = 1;
		break;
	}
}

void swingdacrunch(PLAYER& plr, int daweapon) {

	switch (daweapon) {
	case 0: // fist
		spritesound(S_SOCK1 + (krand() % 4), &sprite[plr.spritenum]);
		break;
	case 1: // dagger
		if ((krand() % 2) != 0)
			spritesound(S_GORE1 + (krand() % 4), &sprite[plr.spritenum]);
		break;
	case 2: // short sword
		spritesound(S_SWORD2 + (krand() % 3), &sprite[plr.spritenum]);
		break;
	case 3: // morningstar
		spritesound(S_SOCK1 + (krand() % 4), &sprite[plr.spritenum]);
		break;
	case 4: // broad sword
		spritesound(S_SWORD1 + (krand() % 3), &sprite[plr.spritenum]);
		break;
	case 5: // battle axe
		if ((krand() % 2) != 0)
			spritesound(S_SOCK1 + (krand() % 4), &sprite[plr.spritenum]);
		else
			spritesound(S_SWORD1 + (krand() % 3), &sprite[plr.spritenum]);
		break;
	case 6: // bow

		break;
	case 7: // pike
		if ((krand() % 2) != 0)
			spritesound(S_SOCK1 + (krand() % 4), &sprite[plr.spritenum]);
		else
			spritesound(S_SWORD1 + (krand() % 3), &sprite[plr.spritenum]);
		break;
	case 8: // two handed sword
		spritesound(S_SWORD1 + (krand() % 2), &sprite[plr.spritenum]);
		break;
	case 9: // halberd
		if ((krand() % 2) != 0)
			spritesound(S_SOCK1 + (krand() % 4), &sprite[plr.spritenum]);
		else
			spritesound(S_SWORD1 + (krand() % 3), &sprite[plr.spritenum]);
		break;
	}
}

void swingdasound(int daweapon, boolean enchanted) {

	switch (daweapon) {
	case 0: // fist
		SND_Sound(S_PLRWEAPON0);
		break;
	case 1: // knife
		SND_Sound(S_PLRWEAPON1);
		break;
	case 2: // short sword
		if (isWh2() && enchanted)
			SND_Sound(S_FIRESWING);
		else
			SND_Sound(S_PLRWEAPON4);
		break;
	case 3: // mace
		if (isWh2() && enchanted) {
			SND_Sound(S_FIRESWING);
			SND_Sound(S_FIREBALL);
		} else 
			SND_Sound(S_PLRWEAPON2);
		break;
	case 4: //
		SND_Sound(S_PLRWEAPON4);
		break;
	case 5: // sword
		SND_Sound(S_PLRWEAPON4);
		break;
	case 6: // bow
		SND_Sound(S_PLRWEAPON3);
		break;
	case 7: //
		if (isWh2() && enchanted)
			SND_Sound(S_ENERGYSWING);
		else
			SND_Sound(S_PLRWEAPON4);
		break;
	case 8: //
		SND_Sound(S_PLRWEAPON4);
		break;
	case 9: //
		SND_Sound(S_PLRWEAPON4);
		break;
	}
}

END_WH_NS
