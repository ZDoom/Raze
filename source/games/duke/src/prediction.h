#pragma once

BEGIN_DUKE_NS

extern DVector3 omypos, mypos;
extern int myxvel, myyvel, myzvel;
extern int globalskillsound;
extern int mycursectnum, myjumpingcounter;
extern DAngle myang, omyang;
extern fixedhoriz myhoriz, omyhoriz, myhorizoff, omyhorizoff;
extern uint8_t myjumpingtoggle, myonground, myhardlanding,myreturntocenter;
extern int fakemovefifoplc;
extern int myxbak[MOVEFIFOSIZ], myybak[MOVEFIFOSIZ], myzbak[MOVEFIFOSIZ];
extern int myhorizbak[MOVEFIFOSIZ];
extern short myangbak[MOVEFIFOSIZ];

END_DUKE_NS
