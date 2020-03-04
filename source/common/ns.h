#pragma once

#ifndef NO_NAMESPACE

#define BEGIN_DUKERR_NS namespace DukeRR {
#define END_DUKERR_NS }

#define BEGIN_DUKE_NS namespace DukeRR{} namespace Duke { using namespace DukeRR;
#define END_DUKE_NS }

#define BEGIN_RR_NS namespace DukeRR{} namespace Redneck { using namespace DukeRR;
#define END_RR_NS }

#define BEGIN_BLD_NS namespace Blood {
#define END_BLD_NS }

#define BEGIN_SW_NS namespace ShadowWarrior {
#define END_SW_NS }

#define BEGIN_PS_NS namespace Powerslave {
#define END_PS_NS }

#else

#define BEGIN_DUKERR_NS
#define END_DUKERR_NS

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

