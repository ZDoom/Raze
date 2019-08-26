
#ifndef __network_h__
#define __network_h__

extern short nNetMoveFrames;

void SendGoodbye();
void UpdateNetInputs();
int InitNet(short nSocket, int nPlayers);
int InitSerial();
void AbortNetworkPlay();
void UnInitNet();

#endif
