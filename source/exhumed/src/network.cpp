
// this is net.c in the original code

#include "typedefs.h"
#include "network.h"
#include "serial.h"
#include "input.h"

short nNetMoveFrames = 0;


void SendGoodbye()
{
	bSendBye = kTrue;
	UpdateInputs();
}

void UpdateNetInputs()
{

}

int InitNet(short nSocket, int nPlayers)
{
	return 0;
}

int InitSerial()
{
	return 1;
}

void AbortNetworkPlay()
{

}

void UnInitNet()
{

}
