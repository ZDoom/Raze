#define ENET_IMPLEMENTATION
#include "enet.h"
#include "printf.h"

bool gHaveNetworking;

void ShutdownENet()
{
	if (gHaveNetworking) enet_deinitialize();
}

void InitENet()
{
	gHaveNetworking = !enet_initialize();
	if (!gHaveNetworking)
		Printf("An error occurred while initializing ENet.\n");
}

