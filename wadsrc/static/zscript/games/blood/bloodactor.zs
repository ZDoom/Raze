
struct XSPRITE {

	//AISTATE* aiState;                   // ai
	uint flags;

	native BloodActor target;           // target sprite
	native BloodActor burnSource;
	
	native Vector3 TargetPos;
	native double goalAng;          // Dude goal ang

	native int sysData1;            // used to keep here various system data, so user can't change it in map editor
	native int sysData2;            //
	native int scale;                   // used for scaling SEQ size on sprites
	native uint physAttr;         // currently used by additional physics sprites to keep its attributes.
	native uint health;
	native uint busy;

	native int16 data1;            // Data 1
	native int16 data2;            // Data 2
	native int16 data3;            // Data 3
	native uint16 txID;             // TX ID
	native uint16 rxID;             // RX ID
	native uint16 command;           // Cmd
	native uint16 busyTime;         // busyTime
	native uint16 waitTime;         // waitTime
	native uint16 data4;            // Data 4
	native uint16 burnTime;
	native uint16 height;
	native uint16 stateTimer;       // ai timer

	native uint8 respawnPending;    // respawnPending
	native uint8 dropMsg;           // Drop Item
	native uint8 key;               // Key
	native uint8 lSkill;            // Launch 12345
	native uint8 lockMsg;           // Lock msg
	native int8 dodgeDir;          // Dude dodge direction
	native uint8 wave;              // Wave
	native uint8 medium;            // medium
	native uint8 respawn;           // Respawn option

	native uint8 modernFlags;            // modern flags
	native uint8 sightstuff;           // something about sight checks
	native uint8 patrolturndelay;           // patrol turn delay

	flagdef internal state: flags, 0;
	flagdef internal triggerOn: flags, 1;
	flagdef internal triggerOff: flags, 2;
	flagdef internal restState: flags, 3;
	flagdef internal interruptable: flags, 4;
	flagdef internal Decoupled: flags, 5;
	flagdef internal triggerOnce: flags, 6;
	flagdef internal isTriggered: flags, 7;
	flagdef internal Push: flags, 8;
	flagdef internal Vector: flags, 9;
	flagdef internal Impact: flags, 10;
	flagdef internal Pickup: flags, 11;
	flagdef internal Touch: flags, 12;
	flagdef internal Sight: flags, 13;
	flagdef internal Proximity: flags, 14;
	flagdef internal lS: flags, 15;
	flagdef internal lB: flags, 16;
	flagdef internal lT: flags, 17;
	flagdef internal lC: flags, 18;
	flagdef internal DudeLockout: flags, 19;
	flagdef internal locked: flags, 20;
	flagdef internal dudeDeaf: flags, 21;
	flagdef internal dudeAmbush: flags, 22;
	flagdef internal dudeGuard: flags, 23;
	flagdef internal dudeFlag4: flags, 24;
	flagdef internal patrolstate: flags, 25;
}

struct SPRITEHIT
{
	native CollisionData hit, ceilhit, florhit;
}

struct DUDEEXTRA
{
	native int time;
	native uint8 teslaHit;
	native uint8 active;
	native int prio;
	native int thinkTime;
	native int birthCounter;
}


class BloodActor : CoreActor native
{
	meta int defshade;
	meta int defpal;
	Property prefix: none;
	property shade: defshade;
	property pal: defpal;
	
	enum GAME_TYPE
	{
		kSingleplayer = 1,
		kDeathmatch = 2,
		kTeamplay = 3
	}
	
	enum STAT_ID {
		kStatDecoration = 0,
		kStatFX = 1,
		kStatExplosion = 2,
		kStatItem = 3,
		kStatThing = 4,
		kStatProjectile = 5,
		kStatDude = 6,
		kStatInactive = 7, // inactive (ambush) dudes
		kStatRespawn = 8,
		kStatPurge = 9,
		kStatMarker = 10,
		kStatTraps = 11,
		kStatAmbience = 12,
		kStatSpares = 13,
		kStatFlare = 14,
		kStatDebris = 15,
		kStatPathMarker = 16,
		kStatFree = 1024,
	};
	
	enum CALLBACK_ID {
		kCallbackNone = -1,
		kCallbackFXFlameLick = 0,
		kCallbackRemove = 1,
		kCallbackFXFlareBurst = 2,
		kCallbackFXFlareSpark = 3,
		kCallbackFXFlareSparkLite = 4,
		kCallbackFXZombieSpurt = 5,
		kCallbackFXBloodSpurt = 6,
		kCallbackFXArcSpark = 7,
		kCallbackFXDynPuff = 8,
		kCallbackRespawn = 9,
		kCallbackPlayerBubble = 10,
		kCallbackEnemeyBubble = 11,
		kCallbackCounterCheck = 12,
		kCallbackFinishHim = 13,
		kCallbackFXBloodBits = 14,
		kCallbackFXTeslaAlt = 15,
		kCallbackFXBouncingSleeve = 16,
		kCallbackReturnFlag = 17,
		kCallbackFXPodBloodSpray = 18,
		kCallbackFXPodBloodSplat = 19,
		kCallbackLeechStateTimer = 20,
		kCallbackDropVoodoo = 21, // unused

		kCallbackMissileBurst = 22,
		kCallbackMissileSpriteBlock = 23,
		kCallbackGenDudeUpdate = 24,
		kCallbackCondition = 25,
	}
	
	native double dudeSlope;
	native readonly bool hasx;
	native bool explosionhackflag; // this originally hijacked the target field which is not safe when working with pointers.
	native XSPRITE xspr;
	native SPRITEHIT hit;
	native DUDEEXTRA dudeExtra;
	native BloodActor ownerActor;	// was previously stored in the sprite's owner field.

	// nnext stuff. For now not exported to scripting.
	//SPRITEMASS spriteMass;
	//GENDUDEEXTRA genDudeExtra;
	//TObjPtr<DBloodActor*> prevmarker;	// needed by the nnext marker code. This originally hijacked targetX in XSPRITE
	//DVector3 basePoint;
	//EventObject condition[2];

	// transient data (not written to savegame)
	native int cumulDamage;
	native bool interpolated;


	native void ChangeType(class<BloodActor> newtype);
	native static BloodActor InsertSprite(sectortype pSector, int nStat, Class<BloodActor> cls);
	native void addX();
	native void evPostActorCallback(int delta, int callback);
	native double, double getActorExtents();
	
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static BloodActor spawnSprite(sectortype pSector, Vector3 pos, int nStat, bool setextra, Class<BloodActor> cls)
	{
		let spawned = InsertSprite(pSector, nStat, cls);
		spawned.lotag = Raze.getSpawnNum(cls);	// we still need this, mapping may not be ambiguous.

		spawned.setposition(pos);
		if (setextra && !spawned.hasX)
		{
			spawned.addX();
			spawned.hit.florhit.setNone();
			spawned.hit.ceilhit.setNone();
		}
		return spawned;
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	BloodActor dropObject(class<BloodActor> itemtype)
	{
		if (!(itemtype is 'BloodItemBase') &&
			!(itemtype is 'BloodAmmoBase') &&
			!(itemtype is 'BloodWeaponBase')) return null;

		let pos = self.pos;
		let sector = Raze.updatesector(pos.XY, self.sector);
		double c,f;
		[c, f] = sector.getSlopes(pos.XY);
		
		let spawned = spawnSprite(sector, (pos.xy, f), kStatItem, false, itemtype);
		if (!spawned) return null;
		
		spawned.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		spawned.shade = spawned.defshade;
		
		if (itemtype is 'BloodKeyBase' && gGameOptions.nGameType == kSingleplayer)
		{
			spawned.xspr.respawn = 3;
		}
		if (itemtype is 'BloodFlagBase' && gGameOptions.nGameType == kTeamplay)
		{
			spawned.evPostActorCallback(1800, kCallbackReturnFlag);
		}

		double top, bottom;
		[top, bottom] = spawned.GetActorExtents();
		if (bottom >= spawned.pos.Z)
			spawned.pos.Z -= (bottom - spawned.pos.Z);

		return spawned;
	}


}
