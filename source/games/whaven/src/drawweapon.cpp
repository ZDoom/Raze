#include "ns.h"
#include "wh.h"
#include "v_font.h"
#include "v_draw.h"
#include "gamehud.h"
#include "statusbar.h"
#include "mapinfo.h"

BEGIN_WH_NS

static void overwritesprite(double thex, double they, int tilenum, int shade, int stat, int dapalnum) 
{
	int dastat = (((stat & RS_TRANS1) ^ RS_TRANS1) << 4) + (stat & RS_AUTO) + ((stat & RS_YFLIP) >> 2) + RS_NOCLIP +
	             (((stat & RS_TOPLEFT) >> 2) ^ ((stat & RS_NOCLIP) >> 1)) + (stat & RS_ALIGN_L) + (stat & RS_ALIGN_R);
	hud_drawsprite(thex, they, 65536, (stat & RS_NOCLIP) << 7, tilenum, shade, dapalnum, dastat);
}

void drawweapons(int snum, double const dasmoothratio) {
	double dax, day;
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

	// Interpoplated snake values for smooth bobbing.
	double dasnakex = osnakex + MulScaleF(snakex - osnakex, dasmoothratio, 16);
	double dasnakey = osnakey + MulScaleF(snakey - osnakey, dasmoothratio, 16);

	// Interpolated weapon dropping.
	double daweapondrop = oweapondrop + MulScaleF(weapondrop - oweapondrop, dasmoothratio, 16);

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
						day = readyanimtics[plr.currweapon][6].curry + dasnakey + 8;
						dax = readyanimtics[plr.currweapon][6].currx + dasnakex + 8;
					}
					else {
						day = zreadyanimtics[plr.currweapon][6].curry + dasnakey + 8;
						dax = zreadyanimtics[plr.currweapon][6].currx + dasnakex + 8;
					}
				}
				else {
					day = readyanimtics[plr.currweapon][6].curry + dasnakey + 8;
					dax = readyanimtics[plr.currweapon][6].currx + dasnakex + 8;
				}
			}
			else {
				if (isWh2()) {
					if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
						day = weaponanimtics[plr.currweapon][0].curry + dasnakey + 8;
						dax = weaponanimtics[plr.currweapon][0].currx + dasnakex + 8;
					}
					else {
						day = zweaponanimtics[plr.currweapon][0].curry + dasnakey + 8;
						dax = zweaponanimtics[plr.currweapon][0].currx + dasnakex + 8;
					}
				}
				else {
					day = weaponanimtics[plr.currweapon][0].curry + dasnakey + 8;
					dax = weaponanimtics[plr.currweapon][0].currx + dasnakex + 8;
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
			overwritesprite(dax + dasnakex, day, plr.currweaponframe, dashade, dabits, dapalnum);
		break;
	case 2: // unready
		if (isWh2()) {
			if (plr.currweaponframe == BOWREADYEND) {
				day = readyanimtics[plr.currweapon][6].curry + daweapondrop;
				dax = readyanimtics[plr.currweapon][6].currx;
			}
			else if (plr.currweaponframe == ZBOWWALK) {
				day = zreadyanimtics[plr.currweapon][6].curry + daweapondrop;
				dax = zreadyanimtics[plr.currweapon][6].currx;
			}
			else {
				if (plr.weapon[plr.currweapon] == 1 || plr.weapon[7] == 2) {
					dax = weaponanimtics[plr.currweapon][0].currx;
					day = weaponanimtics[plr.currweapon][0].curry + daweapondrop;
				}
				else {
					dax = zweaponanimtics[plr.currweapon][0].currx;
					day = zweaponanimtics[plr.currweapon][0].curry + daweapondrop;
				}
			}
		}
		else {
			if (plr.currweaponframe == BOWREADYEND) {
				day = readyanimtics[plr.currweapon][6].curry + daweapondrop;
				dax = readyanimtics[plr.currweapon][6].currx;
			}
			else {
				dax = weaponanimtics[plr.currweapon][0].currx;
				day = weaponanimtics[plr.currweapon][0].curry + daweapondrop;
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
				overwritesprite(-40 + dasnakex, 100 + dasnakey, GRONSHIELD, dashade, dabits, dapalnum);
			}
			else if (plr.shieldpoints > 50 && plr.shieldpoints < 76) {
				overwritesprite(-40 + dasnakex, 100 + dasnakey, GRONSHIELD + 1, dashade, dabits, dapalnum);
			}
			else if (plr.shieldpoints > 25 && plr.shieldpoints < 51) {
				overwritesprite(-40 + dasnakex, 100 + dasnakey, GRONSHIELD + 2, dashade, dabits, dapalnum);
			}
			else {
				overwritesprite(-40 + dasnakex, 100 + dasnakey, GRONSHIELD + 3, dashade, dabits, dapalnum);
			}
		}
		else {
			if (plr.shieldpoints > 150) {
				overwritesprite(-40 + dasnakex, 100 + dasnakey, ROUNDSHIELD, dashade, dabits, dapalnum);
			}
			else if (plr.shieldpoints > 100 && plr.shieldpoints < 151) {
				overwritesprite(-40 + dasnakex, 100 + dasnakey, ROUNDSHIELD + 1, dashade, dabits, dapalnum);
			}
			else if (plr.shieldpoints > 50 && plr.shieldpoints < 101) {
				overwritesprite(-40 + dasnakex, 100 + dasnakey, ROUNDSHIELD + 2, dashade, dabits, dapalnum);
			}
			else {
				overwritesprite(-40 + dasnakex, 100 + dasnakey, ROUNDSHIELD + 3, dashade, dabits, dapalnum);
			}
		}
	}
}

void spikeanimation(PLAYER& plr) 
{
	const int wait = 2;	// was 4, but called from the render code running at fixed 60 fps.
	if (plr.spiketics < 0) 
	{
		plr.currspikeframe++;
		if (plr.currspikeframe > wait)
			plr.currspikeframe = wait;
		plr.spiketics = spikeanimtics[plr.currspikeframe].daweapontics;
		plr.spikeframe = spikeanimtics[plr.currspikeframe].daweaponframe;
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


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void UpdateFrame(void)
{
	static const int kBackTile = 3;
	auto tex = tileGetTexture(kBackTile);

	twod->AddFlatFill(0, 0, xdim, windowxy1.y - 3, tex);
	twod->AddFlatFill(0, windowxy2.y + 4, xdim, ydim, tex);
	twod->AddFlatFill(0, windowxy1.y - 3, windowxy1.x - 3, windowxy2.y + 4, tex);
	twod->AddFlatFill(windowxy2.x + 4, windowxy1.y - 3, xdim, windowxy2.y + 4, tex);

	twod->AddFlatFill(windowxy1.x - 3, windowxy1.y - 3, windowxy1.x, windowxy2.y + 1, tex, 0, 1, 0xff545454);
	twod->AddFlatFill(windowxy1.x, windowxy1.y - 3, windowxy2.x + 4, windowxy1.y, tex, 0, 1, 0xff545454);
	twod->AddFlatFill(windowxy2.x + 1, windowxy1.y, windowxy2.x + 4, windowxy2.y + 4, tex, 0, 1, 0xff2a2a2a);
	twod->AddFlatFill(windowxy1.x - 3, windowxy2.y + 1, windowxy2.x + 1, windowxy2.y + 4, tex, 0, 1, 0xff2a2a2a);
}


void DrawHud(double const dasmoothratio) 
{
	if (!player[pyrn].dead)
		drawweapons(pyrn, dasmoothratio);
	if (player[pyrn].spiked == 1)
		spikeheart(player[pyrn]);
	if (scarytime >= 0)
		drawscary();

	if (hud_size <= Hud_Stbar)
	{
		UpdateFrame();
	}

	SummaryInfo info{};
	info.kills = kills;
	info.maxkills = killcnt;
	info.secrets = treasuresfound;
	info.maxsecrets = treasurescnt;
	info.supersecrets = 0;
	info.time = Scale(PlayClock, 1000, 120);

	UpdateStatusBar(&info);
}

ReservedSpace GameInterface::GetReservedScreenSpace(int viewsize)
{
	return { 0, tileHeight(SSTATUSBAR) * 200 / 480 };
}


END_WH_NS
