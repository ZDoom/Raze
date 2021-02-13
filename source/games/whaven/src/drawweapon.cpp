#include "ns.h"
#include "wh.h"
#include "v_font.h"
#include "v_draw.h"
#include "glbackend/glbackend.h"

BEGIN_WH_NS

static void overwritesprite(double thex, double they, int tilenum, int shade, int stat, int dapalnum) 
{
	int dastat = (((stat & RS_TRANS1) ^ RS_TRANS1) << 4) + (stat & RS_AUTO) + ((stat & RS_YFLIP) >> 2) + RS_NOCLIP +
	             (((stat & RS_TOPLEFT) >> 2) ^ ((stat & RS_NOCLIP) >> 1)) + (stat & RS_ALIGN_L) + (stat & RS_ALIGN_R);
	hud_drawsprite(thex, they, 65536, (stat & RS_NOCLIP) << 7, tilenum, shade, dapalnum, dastat);
}

void drawweapons(int snum) {

	int dax, day;
	int dashade;
	int dapalnum;

	PLAYER& plr = player[snum];

	if (plr.shadowtime > 0 || plr.sector == -1) {
		dashade = 31;
		dapalnum = 0;
	}
	else {
		dashade = sector[plr.sector].ceilingshade;
		dapalnum = 0;
	}

	int dabits;
	if (plr.invisibletime > 0)
		dabits = RS_TRANS1;
	else
		dabits = 0;

	switch (plr.currweapon) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 6:
	case 9:
		dabits |= RS_ALIGN_R;
		break;
	}

	switch (plr.currweaponfired) {
	case 6:
		if (isWh2()) {
			if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
				{
					dax = lefthandanimtics[plr.currweapon][plr.currweaponanim].currx;
					day = lefthandanimtics[plr.currweapon][plr.currweaponanim].curry + 8;
				}
			}
			else {
				dax = zlefthandanimtics[plr.currweapon][plr.currweaponanim].currx;
				day = zlefthandanimtics[plr.currweapon][plr.currweaponanim].curry + 8;
			}
		}
		else {
			dax = lefthandanimtics[plr.currweapon][plr.currweaponanim].currx;
			day = lefthandanimtics[plr.currweapon][plr.currweaponanim].curry + 8;
		}

		dabits &= ~RS_ALIGN_R;
		if (plr.currweapon == 0 && plr.currweaponframe != 0) {
			if (dahand == 1)
				overwritesprite(dax, day, plr.currweaponframe, dashade, dabits, dapalnum);
			else if (dahand == 2) {
				dax = lefthandanimtics[0][plr.currweaponanim].currx;
				day = lefthandanimtics[0][plr.currweaponanim].curry + 8;
				overwritesprite(dax, day + 5, plr.currweaponframe + 6, dashade, dabits, dapalnum);
			}
		}
		else {
			if (plr.currweaponframe != 0) {
				if (isWh2()) {
					if (plr.weapon[plr.currweapon] == 1)
						dax = lefthandanimtics[plr.currweapon][plr.currweaponanim].currx;
					else
						dax = zlefthandanimtics[plr.currweapon][plr.currweaponanim].currx;
				}
				else
					dax = lefthandanimtics[plr.currweapon][plr.currweaponanim].currx;
				overwritesprite(dax, day, plr.currweaponframe, dashade, dabits, dapalnum);
			}
		}
		break;
	case 1: // fire
		if (plr.currweaponattackstyle == 0) {
			if (isWh2()) {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
					dax = weaponanimtics[plr.currweapon][plr.currweaponanim].currx + 8;
					day = weaponanimtics[plr.currweapon][plr.currweaponanim].curry;
				}
				else {
					dax = zweaponanimtics[plr.currweapon][plr.currweaponanim].currx + 8;
					day = zweaponanimtics[plr.currweapon][plr.currweaponanim].curry;
				}
			}
			else {
				dax = weaponanimtics[plr.currweapon][plr.currweaponanim].currx;
				day = weaponanimtics[plr.currweapon][plr.currweaponanim].curry + 4;
			}
		}
		else {
			if (isWh2()) {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
					dax = weaponanimtics2[plr.currweapon][plr.currweaponanim].currx + 8;
					day = weaponanimtics2[plr.currweapon][plr.currweaponanim].curry;
				}
				else {
					dax = zweaponanimtics2[plr.currweapon][plr.currweaponanim].currx + 8;
					day = zweaponanimtics2[plr.currweapon][plr.currweaponanim].curry;
				}
			}
			else {
				dax = weaponanimtics2[plr.currweapon][plr.currweaponanim].currx;
				day = weaponanimtics2[plr.currweapon][plr.currweaponanim].curry + 4;
			}
		}

		if (plr.currweapon == 0 && plr.currweaponframe != 0) {
			if (dahand == 1) {
				overwritesprite(dax, day, plr.currweaponframe, dashade, dabits, dapalnum);
			}
			else if (dahand == 2) {
				dax = lefthandanimtics[0][plr.currweaponanim].currx;
				day = lefthandanimtics[0][plr.currweaponanim].curry + 8;
				overwritesprite(dax, day + 5, plr.currweaponframe + 6, dashade, dabits, dapalnum);
			}
		}
		else {
			if (plr.currweaponframe != 0) {
				if (plr.currweaponattackstyle == 0) {
					if (isWh2()) {
						// flip
						if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2)
							dax = weaponanimtics[plr.currweapon][plr.currweaponanim].currx;
						else
							dax = zweaponanimtics[plr.currweapon][plr.currweaponanim].currx;
					}
					else
						dax = weaponanimtics[plr.currweapon][plr.currweaponanim].currx;
				}
				else {
					if (isWh2()) {
						if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2)
							dax = weaponanimtics2[plr.currweapon][plr.currweaponanim].currx;
						else
							dax = zweaponanimtics2[plr.currweapon][plr.currweaponanim].currx;
					}
					else
						dax = weaponanimtics2[plr.currweapon][plr.currweaponanim].currx;
				}
				overwritesprite(dax, day, plr.currweaponframe, dashade, dabits, dapalnum);
			}
		}
		break;

	case 0: // walking
		if ((plr.plInput.fvel | plr.plInput.svel) != 0) {
			if (plr.currweaponframe == BOWREADYEND) {
				if (isWh2()) {
					if (plr.weapon[plr.currweapon] == 1) {
						day = readyanimtics[plr.currweapon][6].curry + snakey + 8;
						dax = readyanimtics[plr.currweapon][6].currx + snakex + 8;
					}
					else {
						day = zreadyanimtics[plr.currweapon][6].curry + snakey + 8;
						dax = zreadyanimtics[plr.currweapon][6].currx + snakex + 8;
					}
				}
				else {
					day = readyanimtics[plr.currweapon][6].curry + snakey + 8;
					dax = readyanimtics[plr.currweapon][6].currx + snakex + 8;
				}
			}
			else {
				if (isWh2()) {
					if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
						day = weaponanimtics[plr.currweapon][0].curry + snakey + 8;
						dax = weaponanimtics[plr.currweapon][0].currx + snakex + 8;
					}
					else {
						day = zweaponanimtics[plr.currweapon][0].curry + snakey + 8;
						dax = zweaponanimtics[plr.currweapon][0].currx + snakex + 8;
					}
				}
				else {
					day = weaponanimtics[plr.currweapon][0].curry + snakey + 8;
					dax = weaponanimtics[plr.currweapon][0].currx + snakex + 8;
				}
			}
		}
		else {
			if (isWh2()) {
				if (plr.currweaponframe == BOWREADYEND) {
					if (plr.weapon[plr.currweapon] == 1) {
						day = readyanimtics[plr.currweapon][6].curry + 3;
						dax = readyanimtics[plr.currweapon][6].currx + 3;
					}
					else {
						day = zreadyanimtics[plr.currweapon][6].curry + 3;
						dax = zreadyanimtics[plr.currweapon][6].currx + 3;
					}
				}
				else {
					if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
						dax = weaponanimtics[plr.currweapon][0].currx + 3;
						day = weaponanimtics[plr.currweapon][0].curry + 3;
					}
					else {
						dax = zweaponanimtics[plr.currweapon][0].currx + 3;
						day = zweaponanimtics[plr.currweapon][0].curry + 3;
					}
				}
			}
			else {
				if (plr.currweaponframe == BOWREADYEND) {

					day = readyanimtics[plr.currweapon][6].curry + 3;
					dax = readyanimtics[plr.currweapon][6].currx + 3;
				}
				else {
					dax = weaponanimtics[plr.currweapon][0].currx + 3;
					day = weaponanimtics[plr.currweapon][0].curry + 3;
				}
			}
		}

		if (plr.currweapon == 0 && plr.currweaponframe != 0) {
			overwritesprite(dax, day, plr.currweaponframe, dashade, dabits, dapalnum);
			overwritesprite(0, day + 8, plr.currweaponframe + 6, dashade, dabits, dapalnum);
		}
		else if (plr.currweaponframe != 0)
			overwritesprite(dax + snakex, day, plr.currweaponframe, dashade, dabits, dapalnum);
		break;
	case 2: // unready
		if (isWh2()) {
			if (plr.currweaponframe == BOWREADYEND) {
				day = readyanimtics[plr.currweapon][6].curry + (weapondrop);
				dax = readyanimtics[plr.currweapon][6].currx;
			}
			else if (plr.currweaponframe == ZBOWWALK) {
				day = zreadyanimtics[plr.currweapon][6].curry + (weapondrop);
				dax = zreadyanimtics[plr.currweapon][6].currx;
			}
			else {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
					dax = weaponanimtics[plr.currweapon][0].currx;
					day = weaponanimtics[plr.currweapon][0].curry + (weapondrop);
				}
				else {
					dax = zweaponanimtics[plr.currweapon][0].currx;
					day = zweaponanimtics[plr.currweapon][0].curry + (weapondrop);
				}
			}
		}
		else {
			if (plr.currweaponframe == BOWREADYEND) {
				day = readyanimtics[plr.currweapon][6].curry + (weapondrop);
				dax = readyanimtics[plr.currweapon][6].currx;
			}
			else {
				dax = weaponanimtics[plr.currweapon][0].currx;
				day = weaponanimtics[plr.currweapon][0].curry + (weapondrop);
			}
		}

		if (plr.currweapon == 0 && plr.currweaponframe != 0) {
			overwritesprite(dax, day, plr.currweaponframe, dashade, dabits, dapalnum);
			overwritesprite(0, day, plr.currweaponframe + 6, dashade, dabits, dapalnum);
		}
		else if (plr.currweaponframe != 0) {
			dax = weaponanimtics[plr.currweapon][0].currx;
			overwritesprite(dax, day, plr.currweaponframe, dashade, dabits, dapalnum);
		}
		break;
	case 3: // ready
		if (isWh2()) {
			if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
				dax = readyanimtics[plr.currweapon][plr.currweaponanim].currx;
				day = readyanimtics[plr.currweapon][plr.currweaponanim].curry + 8;
			}
			else {
				dax = zreadyanimtics[plr.currweapon][plr.currweaponanim].currx;
				day = zreadyanimtics[plr.currweapon][plr.currweaponanim].curry + 8;
			}
		}
		else {
			dax = readyanimtics[plr.currweapon][plr.currweaponanim].currx;
			day = readyanimtics[plr.currweapon][plr.currweaponanim].curry + 8;
		}

		if (plr.currweapon == 0 && plr.currweaponframe != 0) {
			overwritesprite(dax, day, plr.currweaponframe, dashade, dabits, dapalnum);
			overwritesprite(0, day, plr.currweaponframe + 6, dashade, dabits, dapalnum);
		}
		else if (plr.currweaponframe != 0)
			overwritesprite(dax, day, plr.currweaponframe, dashade, dabits, dapalnum);
		break;
	case 5: // cock
		if (isWh2()) {
			if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
				dax = cockanimtics[plr.currweaponanim].currx;
				day = cockanimtics[plr.currweaponanim].curry + 8;
			}
			else {
				if (plr.weapon[plr.currweapon] == 3) {
					dax = zcockanimtics[plr.currweaponanim].currx;
					day = zcockanimtics[plr.currweaponanim].curry + 8;
				}
				else {
					dax = zcockanimtics[plr.currweaponanim].currx;
					day = zcockanimtics[plr.currweaponanim].curry + 8;
				}
			}
		}
		else {
			dax = cockanimtics[plr.currweaponanim].currx;
			day = cockanimtics[plr.currweaponanim].curry + 8;
		}
		if (plr.currweaponframe != 0)
			overwritesprite(dax, day, plr.currweaponframe, dashade, dabits, dapalnum);
		break;
	case 4: // throw the orb
		if (isWh2()) {
			dax = wh2throwanimtics[plr.currentorb][plr.currweaponanim].currx;
			day = wh2throwanimtics[plr.currentorb][plr.currweaponanim].curry + 8;
		}
		else {
			dax = throwanimtics[plr.currentorb][plr.currweaponanim].currx;
			day = throwanimtics[plr.currentorb][plr.currweaponanim].curry + 8;
		}
		if (plr.currweaponframe != 0)
			overwritesprite(dax, day, plr.currweaponframe, dashade, dabits, dapalnum);
		break;
	}

	// shield stuff

	if (plr.shieldpoints > 0 && (plr.currweaponfired == 0 || plr.currweaponfired == 1) && plr.selectedgun > 0
		&& plr.selectedgun < 5) {
		if (plr.shieldtype == 1) {
			if (plr.shieldpoints > 75) {
				overwritesprite(-40 + snakex, 100 + snakey, GRONSHIELD, dashade, dabits, dapalnum);
			}
			else if (plr.shieldpoints > 50 && plr.shieldpoints < 76) {
				overwritesprite(-40 + snakex, 100 + snakey, GRONSHIELD + 1, dashade, dabits, dapalnum);
			}
			else if (plr.shieldpoints > 25 && plr.shieldpoints < 51) {
				overwritesprite(-40 + snakex, 100 + snakey, GRONSHIELD + 2, dashade, dabits, dapalnum);
			}
			else {
				overwritesprite(-40 + snakex, 100 + snakey, GRONSHIELD + 3, dashade, dabits, dapalnum);
			}
		}
		else {
			if (plr.shieldpoints > 150) {
				overwritesprite(-40 + snakex, 100 + snakey, ROUNDSHIELD, dashade, dabits, dapalnum);
			}
			else if (plr.shieldpoints > 100 && plr.shieldpoints < 151) {
				overwritesprite(-40 + snakex, 100 + snakey, ROUNDSHIELD + 1, dashade, dabits, dapalnum);
			}
			else if (plr.shieldpoints > 50 && plr.shieldpoints < 101) {
				overwritesprite(-40 + snakex, 100 + snakey, ROUNDSHIELD + 2, dashade, dabits, dapalnum);
			}
			else {
				overwritesprite(-40 + snakex, 100 + snakey, ROUNDSHIELD + 3, dashade, dabits, dapalnum);
			}
		}
	}
}

static void spikeheart(PLAYER& plr) 
{
	int dax = spikeanimtics[plr.currspikeframe].currx;
	int day = spikeanimtics[plr.currspikeframe].curry;

	overwritesprite(dax, day, plr.spikeframe, sector[plr.sector].ceilingshade, 0, 0);
	startredflash(10);
}


void drawscary() {
	int scary = -1;
	if (scarytime > 140 && scarytime < 180) scary = 0;
	if (scarytime > 120 && scarytime < 139) scary = 1;
	if (scarytime > 100 && scarytime < 119) scary = 2;
	if (scarytime > 0 && scarytime < 99) scary = 3;
	double scale = scarysize / 128.;

	DrawTexture(twod, tileGetTexture(SCARY + scary), 160, 100, DTA_FullscreenScale, FSMode_Fit320x200, DTA_CenterOffsetRel, true, DTA_Alpha, 0.33, 
		DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
}



	void DrawHud(float smooth) {
		if (!player[pyrn].dead)
			drawweapons(pyrn);
		if (player[pyrn].spiked == 1)
			spikeheart(player[pyrn]);
		if (scarytime >= 0)
			drawscary();

		//drawInterface(player[pyrn]);
	}
	
END_WH_NS
