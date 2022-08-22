// This is separate because the map loader needs knowledge about these types to allocate their instances.
#pragma once
#include "ns.h"

#pragma pack(push, 1)
// This is the on-disk format. Only Blood still needs this for its retarded encryption that has to read this in as a block so that it can be decoded.
// Keep it local so that the engine's sprite type is no longer limited by file format restrictions.
struct spritetypedisk
{
	int32_t x, y, z;
	uint16_t cstat;
	int16_t picnum;
	int8_t shade;
	uint8_t pal, clipdist, detail;
	uint8_t xrepeat, yrepeat;
	int8_t xoffset, yoffset;
	int16_t sectnum, statnum;
	int16_t ang, owner;
	int16_t index, yvel, inittype;
	int16_t type;
	int16_t hitag;
	int16_t extra;
};

struct sectortypedisk
{
	int16_t wallptr, wallnum;
	int32_t ceilingz, floorz;
	uint16_t ceilingstat, floorstat;
	int16_t ceilingpicnum, ceilingheinum;
	int8_t ceilingshade;
	uint8_t ceilingpal, ceilingxpanning, ceilingypanning;
	int16_t floorpicnum, floorheinum;
	int8_t floorshade;
	uint8_t floorpal, floorxpanning, floorypanning;
	uint8_t visibility, fogpal;
	int16_t type;
	int16_t hitag;
	int16_t extra;
};

struct walltypedisk
{
	int32_t x, y;
	int16_t point2, nextwall, nextsector;
	uint16_t cstat;
	int16_t picnum, overpicnum;
	int8_t shade;
	uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
	int16_t type;
	int16_t hitag;
	int16_t extra;
};

#pragma pack(pop)

BEGIN_BLD_NS

class DBloodActor;
struct AISTATE;

struct XSPRITE {

	AISTATE* aiState;                   // ai
	union
	{
		uint32_t flags;
		struct {
			unsigned int state : 1;             // State 0
			unsigned int triggerOn : 1;         // going ON
			unsigned int triggerOff : 1;        // going OFF
			unsigned int restState : 1;         // restState
			unsigned int Interrutable : 1;      // Interruptable
			unsigned int Decoupled : 1;         // Decoupled
			unsigned int triggerOnce : 1;       // 1-shot
			unsigned int isTriggered : 1;       // works in case if triggerOnce selected
			unsigned int Push : 1;              // Push
			unsigned int Vector : 1;            // Vector
			unsigned int Impact : 1;            // Impact
			unsigned int Pickup : 1;            // Pickup
			unsigned int Touch : 1;             // Touch
			unsigned int Sight : 1;             // Sight
			unsigned int Proximity : 1;         // Proximity
			unsigned int lS : 1;                // Single
			unsigned int lB : 1;                // Bloodbath
			unsigned int lT : 1;                // Launch Team
			unsigned int lC : 1;                // Coop
			unsigned int DudeLockout : 1;       // DudeLockout
			unsigned int locked : 1;            // Locked
			unsigned int dudeDeaf : 1;          // dudeDeaf
			unsigned int dudeAmbush : 1;        // dudeAmbush
			unsigned int dudeGuard : 1;         // dudeGuard
			unsigned int dudeFlag4 : 1;         // unused
			unsigned int wave : 2;              // Wave
			unsigned int medium : 2;            // medium
			unsigned int respawn : 2;           // Respawn option
			unsigned int unused2 : 1;           // (new) patrol state
		};
	};

	TObjPtr<DBloodActor*> target;           // target sprite
	TObjPtr<DBloodActor*> burnSource;
	
	const vec3_t int_TargetPos() const { return { int(TargetPos.X * worldtoint), int(TargetPos.Y * worldtoint), int(TargetPos.Z * worldtoint)}; }

	DVector3 TargetPos;

	int32_t sysData1;            // used to keep here various system data, so user can't change it in map editor
	int32_t sysData2;            //
	int32_t scale;                   // used for scaling SEQ size on sprites
	uint32_t physAttr;         // currently used by additional physics sprites to keep its attributes.
	uint32_t health;
	uint32_t busy;

	int16_t data1;            // Data 1
	int16_t data2;            // Data 2
	int16_t data3;            // Data 3
	uint16_t txID;             // TX ID
	uint16_t rxID;             // RX ID
	uint16_t command;           // Cmd
	uint16_t busyTime;         // busyTime
	uint16_t waitTime;         // waitTime
	uint16_t data4;            // Data 4
	uint16_t goalAng;          // Dude goal ang
	uint16_t burnTime;
	uint16_t height;
	uint16_t stateTimer;       // ai timer

	uint8_t respawnPending;    // respawnPending
	uint8_t dropMsg;           // Drop Item
	uint8_t key;               // Key
	uint8_t lSkill;            // Launch 12345
	uint8_t lockMsg;           // Lock msg
	int8_t dodgeDir;          // Dude dodge direction
	uint8_t unused1;            // modern flags
	uint8_t unused3;           // something about sight checks
	uint8_t unused4;           // patrol turn delay

};

struct XSECTOR {

	union
	{
		uint64_t flags;
		struct {
			unsigned int state : 1;             // State
			unsigned int triggerOn : 1;         // Send at ON
			unsigned int triggerOff : 1;        // Send at OFF
			unsigned int restState : 1;
			unsigned int interruptable : 1;     // Interruptable
			unsigned int reTriggerA : 1;        // OFF->ON wait
			unsigned int reTriggerB : 1;        // ON->OFF wait
			unsigned int shadeAlways : 1;       // Lighting shadeAlways
			unsigned int shadeFloor : 1;        // Lighting floor
			unsigned int shadeCeiling : 1;      // Lighting ceiling
			unsigned int shadeWalls : 1;        // Lighting walls
			unsigned int panAlways : 1;         // Pan always
			unsigned int panFloor : 1;          // Pan floor
			unsigned int panCeiling : 1;        // Pan ceiling
			unsigned int Drag : 1;              // Pan drag
			unsigned int Underwater : 1;        // Underwater
			unsigned int decoupled : 1;         // Decoupled
			unsigned int triggerOnce : 1;       // 1-shot
			unsigned int isTriggered : 1;
			unsigned int Push : 1;              // Push
			unsigned int Vector : 1;            // Vector
			unsigned int Reserved : 1;          // Reserved
			unsigned int Enter : 1;             // Enter
			unsigned int Exit : 1;              // Exit
			unsigned int Wallpush : 1;          // WallPush
			unsigned int color : 1;             // Color Lights
			unsigned int stopOn : 1;
			unsigned int stopOff : 1;
			unsigned int Crush : 1;             // Crush
			unsigned int locked : 1;            // Locked
			unsigned int windAlways : 1;        // Wind always
			unsigned int dudeLockout : 1;
			unsigned int bobAlways : 1;         // Motion always
			unsigned int bobFloor : 1;          // Motion bob floor
			unsigned int bobCeiling : 1;        // Motion bob ceiling
			unsigned int bobRotate : 1;         // Motion rotate
			unsigned int unused1 : 1;           // (new) pause motion

		};
	};
	DBloodActor* marker0;
	DBloodActor* marker1;
	DBloodActor* basePath;
	DBloodActor* actordata;

	uint32_t busy;
	int32_t offCeilZ;
	int32_t onCeilZ;
	int32_t offFloorZ;
	int32_t onFloorZ;
	uint32_t windVel;          // Wind vel (changed from 10 bit to use higher velocity values)

	uint16_t data;             // Data
	uint16_t txID;             // TX ID
	uint16_t rxID;             // RX ID
	uint16_t busyTimeA;        // OFF->ON busyTime
	uint16_t waitTimeA;        // OFF->ON waitTime
	uint16_t panAngle;         // Motion angle
	uint16_t busyTimeB;        // ON->OFF busyTime
	uint16_t waitTimeB;        // ON->OFF waitTime
	uint16_t windAng;          // Wind ang
	uint16_t bobTheta;         // Motion Theta
	int16_t bobSpeed;           // Motion speed

	uint8_t busyWaveA;         // OFF->ON wave
	uint8_t busyWaveB;         // ON->OFF wave
	uint8_t command;           // Cmd
	int8_t amplitude;           // Lighting amplitude
	uint8_t freq;              // Lighting freq
	uint8_t phase;             // Lighting phase
	uint8_t wave;              // Lighting wave
	int8_t shade;               // Lighting value
	uint8_t panVel;            // Motion speed
	uint8_t Depth;             // Depth
	uint8_t Key;               // Key
	uint8_t ceilpal;           // Ceil pal2
	uint8_t damageType;        // DamageType
	uint8_t floorpal;          // Floor pal2
	uint8_t bobZRange;         // Motion Z range
};

struct XWALL {

	union
	{
		uint32_t flags;
		struct {
			unsigned int state : 1;             // State
			unsigned int triggerOn : 1;         // going ON
			unsigned int triggerOff : 1;        // going OFF
			unsigned int restState : 1;         // restState
			unsigned int interruptable : 1;     // Interruptable
			unsigned int panAlways : 1;         // panAlways
			unsigned int decoupled : 1;         // Decoupled
			unsigned int triggerOnce : 1;       // 1-shot
			unsigned int isTriggered : 1;
			unsigned int triggerPush : 1;       // Push
			unsigned int triggerVector : 1;     // Vector
			unsigned int triggerTouch : 1;      // by NoOne: renamed from Reserved to Touch as it works with Touch now.
			unsigned int locked : 1;            // Locked
			unsigned int dudeLockout : 1;       // DudeLockout
		};
	};
	uint32_t busy;

	int16_t data;               // Data
	uint16_t txID;             // TX ID
	uint16_t rxID;             // RX ID
	uint16_t busyTime;         // busyTime
	uint16_t waitTime;         // waitTime

	uint8_t command;           // Cmd
	vec3_t panVel;
	uint8_t key;               // Key
};

END_BLD_NS
