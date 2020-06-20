
#include "types.h"
#include "build.h"
#include "screenjob.h"
#include "dobject.h"


void RunScreen(const char *classname, VMValue *params, int numparams, std::function<void(bool aborted)> completion)
{
	int ticker = 0;
	auto ototalclock = totalclock;
	
	PClass *cls = PClass::FindClass(classname);
	if (cls != nullptr && cls->IsDescendantOf(NAME_GameScreen))
	{
		auto func = dyn_cast<PFunction>(cls->FindSymbol("Init", true));
		if (func != nullptr && !(func->Variants[0].Flags & (VARF_Protected | VARF_Private)))	// skip internal classes which have a protected init method.
		{
			TArray<VMValue> args(numparams+1);
			for(int i = 0;i<numparams;i++) args[i+1] = params[i];
			auto item = cls->CreateNew(); 			
			if (!item) 
			{
				completion(true);
				return;
			}
			args[0] = item;
			VMCallWithDefaults(func->Variants[0].Implementation, args, nullptr, 0); 	
			
			while (true)
			{
				handleevents();
				event_t *ev;
				// This should later run off D_ProcessEvents, but currently requires a local copy here
				while (eventtail != eventhead)
				{
					ev = &events[eventtail];
					eventtail = (eventtail + 1) & (MAXEVENTS - 1);
					if (ev->type == EV_None)
						continue;
					if (ev->type == EV_DeviceChange)
						UpdateJoystickMenu(I_UpdateDeviceList());
#if 0	// this isn't safe with the current engine structure
					if (C_Responder (ev))
						continue;				// console ate the event
					if (M_Responder (ev))
						continue;				// menu ate the event
#endif

					IFVIRTUALPTRNAME(item, NAME_GameScreen, OnEvent)
					{
						FInputEvent e = ev;
						VMValue params[] = { (DObject*)item, &e };
						int retval;
						VMReturn ret(&retval);
						VMCall(func, params, 2, &ret, 1);
						if (!retval)
						{
							completion(true);
							return;
						}
					} 
				}
				
				while ((int)(totalclock - ototalclock) >= 1)
				{
					IFVIRTUALPTRNAME(item, NAME_GameScreen, Tick)
					{
						FInputEvent e = ev;
						VMValue params[] = { (DObject*)item, ticker };
						ticker++;
						int retval;
						VMReturn ret(&retval);
						VMCall(func, params, 2, &ret, 1);
						if (!retval)
						{
							completion(false);
							return;
						}
					} 
				}
				IFVIRTUALPTRNAME(item, NAME_GameScreen, Draw)
				{
					FInputEvent e = ev;
					VMValue params[] = { (DObject*)item };
					ticker++;
					VMCall(func, params, 1, nullptr, 0);
				} 
				videoNextPage();
			}
		}
	}
	completion(true);
}
