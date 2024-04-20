#include "name.h"
#include "dobject.h"

bool ShouldAllowGameSpecificVirtual(FName name, unsigned index, PType* arg, PType* varg)
{
	return false;
}

void DObject::EnableNetworking(const bool enable)
{
	return;
}

void DObject::RemoveFromNetwork(void)
{
	return;
}

void DObject::ClearNetworkID()
{
	return;
}