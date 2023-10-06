
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
	meta double defclipdist;
	Property prefix: none;
	property shade: defshade;
	property pal: defpal;
	property clipdist: defclipdist;
	
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
	
	
// all callbacks, this is to allow using VM functions for all of them
native void aiGenDudeMoveForward();
native void aiMoveDodge();
native void aiMoveForward();
native void aiMoveTurn();
native void aiPodChase();
native void aiPodMove();
native void aiPodSearch();
native void aiThinkTarget();
native void batMoveDodgeDown();
native void batMoveDodgeUp();
native void batMoveFly();
native void batMoveForward();
native void batMoveSwoop();
native void batMoveToCeil();
native void batThinkChase();
native void batThinkGoto();
native void batThinkPonder();
native void batThinkSearch();
native void batThinkTarget();
native void beastMoveForward();
native void beastThinkChase();
native void beastThinkGoto();
native void beastThinkSearch();
native void beastThinkSwimChase();
native void beastThinkSwimGoto();
native void burnThinkChase();
native void burnThinkGoto();
native void burnThinkSearch();
native void calebThinkChase();
native void calebThinkGoto();
native void calebThinkSearch();
native void calebThinkSwimChase();
native void calebThinkSwimGoto();
native void cerberusThinkChase();
native void cerberusThinkGoto();
native void cerberusThinkSearch();
native void cerberusThinkTarget();
native void cultThinkChase();
native void cultThinkGoto();
native void cultThinkSearch();
native void eelMoveAscend();
native void eelMoveDodgeDown();
native void eelMoveDodgeUp();
native void eelMoveForward();
native void eelMoveSwoop();
native void eelMoveToCeil();
native void eelThinkChase();
native void eelThinkGoto();
native void eelThinkPonder();
native void eelThinkSearch();
native void eelThinkTarget();
native void entryAIdle();
native void entryEStand();
native void entryEZombie();
native void entryFStatue();
native void entrySStatue();
native void forcePunch();
native void gargMoveDodgeDown();
native void gargMoveDodgeUp();
native void gargMoveFly();
native void gargMoveForward();
native void gargMoveSlow();
native void gargMoveSwoop();
native void gargThinkChase();
native void gargThinkGoto();
native void gargThinkSearch();
native void gargThinkTarget();
native void ghostMoveDodgeDown();
native void ghostMoveDodgeUp();
native void ghostMoveFly();
native void ghostMoveForward();
native void ghostMoveSlow();
native void ghostMoveSwoop();
native void ghostThinkChase();
native void ghostThinkGoto();
native void ghostThinkSearch();
native void ghostThinkTarget();
native void gillThinkChase();
native void gillThinkGoto();
native void gillThinkSearch();
native void gillThinkSwimChase();
native void gillThinkSwimGoto();
native void handThinkChase();
native void handThinkGoto();
native void handThinkSearch();
native void houndThinkChase();
native void houndThinkGoto();
native void houndThinkSearch();
native void innocThinkChase();
native void innocThinkGoto();
native void innocThinkSearch();
native void MorphToBeast();
native void myThinkSearch();
native void myThinkTarget();
native void playStatueBreakSnd();
native void ratThinkChase();
native void ratThinkGoto();
native void ratThinkSearch();
native void spidThinkChase();
native void spidThinkGoto();
native void spidThinkSearch();
native void sub_628A0();
native void sub_62AE0();
native void sub_62D7C();
native void sub_65D04();
native void sub_65F44();
native void sub_661E0();
native void sub_6CB00();
native void sub_6CD74();
native void sub_6D03C();
native void sub_72580();
native void sub_725A4();
native void sub_72850();
native void tchernobogThinkChase();
native void unicultThinkChase();
native void unicultThinkGoto();
native void unicultThinkSearch();
native void zombaThinkChase();
native void zombaThinkGoto();
native void zombaThinkPonder();
native void zombaThinkSearch();
native void zombfThinkChase();
native void zombfThinkGoto();
native void zombfThinkSearch();
native void FireballSeqCallback();
native void Fx33Callback();
native void NapalmSeqCallback();
native void Fx32Callback();
native void TreeToGibCallback();
native void DudeToGibCallback1();
native void DudeToGibCallback2();
native void batBiteSeqCallback();
native void SlashSeqCallback();
native void StompSeqCallback();
native void eelBiteSeqCallback();
native void BurnSeqCallback();
native void SeqAttackCallback();
native void cerberusBiteSeqCallback();
native void cerberusBurnSeqCallback();
native void cerberusBurnSeqCallback2();
native void TommySeqCallback();
native void TeslaSeqCallback();
native void ShotSeqCallback();
native void cultThrowSeqCallback();
native void cultThrowSeqCallback2();
native void cultThrowSeqCallback3();
native void SlashFSeqCallback();
native void ThrowFSeqCallback();
native void BlastSSeqCallback();
native void ThrowSSeqCallback();
native void ghostSlashSeqCallback();
native void ghostThrowSeqCallback();
native void ghostBlastSeqCallback();
native void GillBiteSeqCallback();
native void HandJumpSeqCallback();
native void houndBiteSeqCallback();
native void houndBurnSeqCallback();
native void podPlaySound1();
native void podPlaySound2();
native void podAttack();
native void podExplode();
native void ratBiteSeqCallback();
native void SpidBiteSeqCallback();
native void SpidJumpSeqCallback();
native void SpidBirthSeqCallback();
native void tchernobogFire();
native void tchernobogBurnSeqCallback();
native void tchernobogBurnSeqCallback2();
native void genDudeAttack1();
native void punchCallback();
native void ThrowCallback1();
native void ThrowCallback2();
native void HackSeqCallback();
native void StandSeqCallback();
native void zombfHackSeqCallback();
native void PukeSeqCallback();
native void ThrowSeqCallback();
native void PlayerSurvive();
native void PlayerKneelsOver();
native void FireballTrapSeqCallback();
native void MGunFireSeqCallback();
native void MGunOpenSeqCallback();
native void fxFlameLick(); // 0
native void RemoveActor(); // 1
native void FlareBurst(); // 2
native void fxFlareSpark(); // 3
native void fxFlareSparkLite(); // 4
native void fxZombieBloodSpurt(); // 5
native void fxBloodSpurt(); // 6
native void fxArcSpark(); // 7
native void fxDynPuff(); // 8
native void Respawn(); // 9
native void PlayerBubble(); // 10
native void EnemyBubble(); // 11
native void FinishHim(); // 13
native void fxBloodBits(); // 14
native void fxTeslaAlt(); // 15
native void fxBouncingSleeve(); // 16
native void returnFlagToBase(); // 17
native void fxPodBloodSpray(); // 18
native void fxPodBloodSplat(); // 19
native void LeechStateTimer(); // 20
native void DropVoodooCb(); // unused	
	
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
	//TObjPtr<BloodActor> prevmarker;	// needed by the nnext marker code. This originally hijacked targetX in XSPRITE
	//DVector3 basePoint;
	//EventObject condition[2];

	// transient data (not written to savegame)
	native int cumulDamage;
	native bool interpolated;


	native void ChangeType(class<BloodActor> newtype);
	native static BloodActor InsertSprite(sectortype pSector, int nStat, Class<BloodActor> cls);
	native void addX();
	native void evPostActorCallback(int delta, VMFunction callback);
	native double, double getActorExtents();
	native int HitScan(double z, vector3 xyz, int clipmask, double clipdist); 
	native void impactMissile(int hitcode);

	native void play3DSoundID(int soundId, int a3 = -1, int a4 = 0);
	native void seqSpawnID(int seqID, VMFunction seqCallbackID);
	
	
	virtual int getRespawnTime()
	{
		return -1;	// no respawn by default.
	}
	
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
		
		if (itemtype is 'BloodKeyBase' && gGameOptions.nGameType == Blood.kSingleplayer)
		{
			spawned.xspr.respawn = 3;
		}
		if (itemtype is 'BloodFlagBase' && gGameOptions.nGameType == Blood.kTeamplay)
		{
			spawned.evPostActorCallback(1800, returnFlagToBase);
		}

		double top, bottom;
		[top, bottom] = spawned.GetActorExtents();
		if (bottom >= spawned.pos.Z)
			spawned.pos.Z -= (bottom - spawned.pos.Z);

		return spawned;
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	BloodActor fireMissile(double xyoff, double zoff, Vector3 dv, class<BloodMissileBase> type)
	{
		if (type == null || !(type is 'BloodMissileBase')) return null;

		bool impact = false;
		Vector3 spawnpos = self.pos + ((self.angle + 90.).ToVector() * xyoff, zoff);

		double clipdist = GetDefaultByType(type).defclipdist + self.clipdist;
		spawnpos.xy += self.angle.ToVector() * clipdist;

		int hit = self.HitScan(spawnpos.Z, (spawnpos.XY - self.pos.XY, 0), CLIPMASK0, clipdist * 4); 
		if (hit != -1)
		{
			if (hit == 3 || hit == 0)
			{
				impact = true;
				spawnpos.XY = gHitInfo.hitpos.XY - self.angle.ToVector() * 1;
			}
			else
			{
				spawnpos.XY = gHitInfo.hitpos.XY - self.angle.ToVector() * GetDefaultByType(type).defclipdist * 2;
			}
		}
		let spawned = BloodMissileBase(self.spawnSprite(self.sector, spawnpos, kStatProjectile, true, type));
		if (spawned == null) return null;

		spawned.cstat2 |= CSTAT2_SPRITE_MAPPED;
		spawned.shade = spawned.defshade;
		spawned.pal = spawned.defpal;
		spawned.clipdist = clipdist;
		spawned.flags = 1;

		spawned.Angle = self.angle + spawned.angleofs;
		spawned.vel = dv.Unit() * spawned.speed;
		spawned.ownerActor = self;
		spawned.cstat |= CSTAT_SPRITE_BLOCK;
		spawned.xspr.target = null;

		spawned.evPostActorCallback(600, RemoveActor);

		spawned.initMissile(self); // handle type specific init.

		if (impact)
		{
			spawned.impactMissile(hit);
			return nullptr;
		}
		return spawned;
	}
}
