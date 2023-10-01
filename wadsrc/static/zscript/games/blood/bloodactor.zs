
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

}
