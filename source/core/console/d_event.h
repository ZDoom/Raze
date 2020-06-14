#pragma once
#include <functional>
#include "d_eventbase.h"


// Called by IO functions when input is detected.
void D_PostEvent (const event_t* ev);

