#pragma once

#ifndef NO_NAMESPACE

#define BEGIN_DUKE_NS namespace Duke3d {
#define END_DUKE_NS }

#define BEGIN_EDUKE_NS namespace Duke {
#define END_EDUKE_NS }

#define BEGIN_RR_NS namespace Redneck {
#define END_RR_NS }

#define BEGIN_BLD_NS namespace Blood {
#define END_BLD_NS }

#define BEGIN_SW_NS namespace ShadowWarrior {
#define END_SW_NS }

#define BEGIN_PS_NS namespace Exhumed {
#define END_PS_NS }

#define BEGIN_WH_NS namespace Witchaven {
#define END_WH_NS }

#else
	
#define BEGIN_EDUKE_NS
#define END_EDUKE_NS

#define BEGIN_DUKE_NS
#define END_DUKE_NS

#define BEGIN_RR_NS
#define END_RR_NS

#define BEGIN_BLD_NS
#define END_BLD_NS

#define BEGIN_SW_NS
#define END_SW_NS

#define BEGIN_PS_NS
#define END_PS_NS

#endif

