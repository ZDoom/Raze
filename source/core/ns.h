#pragma once

#ifndef NO_NAMESPACE

#define DUKE_NS Duke3d
#define BEGIN_DUKE_NS namespace DUKE_NS {
#define END_DUKE_NS }

#define BLD_NS Blood
#define BEGIN_BLD_NS namespace BLD_NS {
#define END_BLD_NS }

#define SW_NS ShadowWarrior
#define BEGIN_SW_NS namespace SW_NS {
#define END_SW_NS }

#define PS_NS Exhumed
#define BEGIN_PS_NS namespace PS_NS {
#define END_PS_NS }

#else

#define BEGIN_DUKE_NS
#define END_DUKE_NS

#define BEGIN_BLD_NS
#define END_BLD_NS

#define BEGIN_SW_NS
#define END_SW_NS

#define BEGIN_PS_NS
#define END_PS_NS

#endif

