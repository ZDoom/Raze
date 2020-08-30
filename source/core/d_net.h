
#ifndef __D_NET__
#define __D_NET__

#include "i_net.h"
#include "d_ticcmd.h"

enum
{
	MAXPLAYERS = 8
};

class FDynamicBuffer
{
public:
	FDynamicBuffer ();
	~FDynamicBuffer ();

	void SetData (const uint8_t *data, int len);
	uint8_t *GetData (int *len = NULL);

private:
	uint8_t *m_Data;
	int m_Len, m_BufferLen;
};

extern FDynamicBuffer NetSpecs[MAXPLAYERS][BACKUPTICS];

// Create any new ticcmds and broadcast to other players.
void NetUpdate (void);

// Broadcasts special packets to other players
//	to notify of game exit
void D_QuitNetGame (void);

//? how many ticks to run?
void TryRunTics (void);

//Use for checking to see if the netgame has stalled
void Net_CheckLastReceived(int);

// [RH] Functions for making and using special "ticcmds"
void Net_NewMakeTic ();
void Net_WriteByte (uint8_t);
void Net_WriteWord (short);
void Net_WriteLong (int);
void Net_WriteFloat (float);
void Net_WriteString (const char *);
void Net_WriteBytes (const uint8_t *, int len);

void Net_DoCommand (int type, uint8_t **stream, int player);
void Net_SkipCommand (int type, uint8_t **stream);

void Net_ClearBuffers ();

bool D_CheckNetGame(void);

void Net_ClearFifo(void);


// Netgame stuff (buffers and pointers, i.e. indices).

// This is the interface to the packet driver, a separate program
// in DOS, but just an abstraction here.
extern	doomcom_t		doomcom;

extern	struct ticcmd_t	localcmds[LOCALCMDTICS];

extern	int 			maketic;
extern	int 			nettics[MAXNETNODES];
extern	int				netdelay[MAXNETNODES][BACKUPTICS];
extern	int 			nodeforplayer[MAXPLAYERS];

extern	ticcmd_t		netcmds[MAXPLAYERS][BACKUPTICS];
extern	int 			ticdup;
extern bool 			nodeingame[MAXNETNODES];				// set false as nodes leave game
extern bool			hadlate;
extern uint64_t		lastglobalrecvtime;						// Identify the last time a packet was received.
extern bool playeringame[MAXPLAYERS]; // as long as network isn't working - true for the first player, false for all others.
extern ticcmd_t playercmds[MAXPLAYERS];
extern short consistancy[MAXPLAYERS][BACKUPTICS];


#endif
