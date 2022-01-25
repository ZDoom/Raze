class DukeActor : CoreActor native
{
	native DukeActor ownerActor, hitOwnerActor;
	native uint8 cgg;
	native uint8 spriteextra;	// moved here for easier maintenance. This was originally a hacked in field in the sprite structure called 'filler'.
	native int16 /*attackertype,*/ hitang, hitextra, movflag;
	native int16 tempang; /*, dispicnum;*/
	native int16 timetosleep;
	native int floorz, ceilingz;
	native int saved_ammo;
	native int palvals;
	native int temp_data[6];
	native private int flags1, flags2;
	//walltype* temp_walls[2]; // SE20 + SE128
	//sectortype* temp_sect, *actorstayput;

	native DukeActor temp_actor, seek_actor;


	flagdef Inventory: flags1, 0;
	flagdef ShrinkAutoaim: flags1, 1;
	flagdef Badguy: flags1, 2;
	flagdef ForceAutoaim: flags1, 3;
	flagdef Boss: flags1, 4;
	flagdef Badguystayput: flags1, 5;
	flagdef GreenSlimeFood: flags1, 6;
	flagdef NoDamagePush: flags1, 7;
	flagdef NoWaterDrip: flags1, 8;
	flagdef InternalBadguy: flags1, 9;
	flagdef Killcount: flags1, 10;
	flagdef NoCanSeeCheck: flags1, 11;
	flagdef HitRadiusCheck: flags1, 12;
	flagdef MoveFTA_CheckSee: flags1, 13;
	flagdef MoveFTA_MakeStandable: flags1, 14;
	flagdef TriggerIfHitSector: flags1, 15;
	//flagdef MoveFTA_WakeupCheck: flags1, 16; // this one needs to be auto-set for RR, not for Duke, should not be exposed unless the feature becomes generally available.
	flagdef CheckSeeWithPal8: flags1, 17;
	flagdef NoShadow: flags1, 18;
	flagdef SE24_NoFloorCheck: flags1, 19;
	flagdef NoInterpolate: flags1, 20;

}
