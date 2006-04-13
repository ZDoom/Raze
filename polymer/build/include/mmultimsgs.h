#ifndef __MMULTIMSGS_H__
#define __MMULTIMSGS_H__

/*
 * Ok, so this header file defines the message bytes and outlines the basic
 * message descriptions for out-of-band messages that are common to all games
 * that utilize my net code. Once a game determines that it is indeed talking
 * to another peer of the same genus, the rest is up to the game itself to
 * decide, but for basic stuff, the interfaces will be identical.
 *
 * Why am I not choosing to implement all this engine-side? Because all the
 * games are different and about the only thing they are guaranteed to use in
 * common that I can be certain of is the services my net code will provide.
 * So, since I can't code anything in particular with every Build game in mind,
 * I'm putting handling all the game-visible messages into the game's domain.
 * The engine will still handle its own internal messages because the game
 * never sees them. Ever.
 *
 * CMDs are messages sent by a peer to another, and RSPs are the replies.
 *
 * The master of the network game, regardless if the eventual game is talking
 * with a peer-to-peer design or not, shall enumerate each peer as it joins
 * and the master will always assign itself peer number 0. This simplifies
 * things all-round because each peer that joins automatically knows that
 * id 0 is its master and it already knows the master's address. Technically
 * every other peer who joins may get a sequential number for its id so maybe
 * even transmitting the peer unique ids is unnecessary and we'd be easier
 * just sending a number of players, but the less craftiness at this point
 * in time, the better.
 *
 *    -- Jonathon
 */

#define MSGPROTOVER		0x00
		// 0x00 20031209


#define MSG_CMD_GETGAMEINFO	0x10
		// char MSG_CMD_GETGAMEINFO
		// char MSGPROTOVER
#define MSG_RSP_BADPROTO	0x11
		// char MSG_RSP_BADPROTO
#define MSG_RSP_NOGAME		0x12
		// char MSG_RSP_NOGAME
		// char[8] gamename
#define MSG_RSP_GAMEINFO	0x13
		// char MSG_RSP_GAMEINFO
		// char[8] gamename	eg. DUKE3DSW/DUKE3D\x00\x00/DUKE3DAT
		// ... other information particular to the game


#define MSG_CMD_JOINGAME	0x20
		// char MSG_CMD_JOINGAME
#define MSG_RSP_GAMEINPROG	0x21
		// char MSG_RSP_GAMEINPROG
#define MSG_RSP_JOINACCEPTED	0x22
		// char MSG_RSP_JOINACCEPTED
		// short uniqueid
		// char numtofollow
		// short[numtofollow] peeruid
		// ... other information particular to the game
#define MSG_RSP_GAMEFULL	0x23
		// char MSG_RSP_GAMEFULL

#endif
