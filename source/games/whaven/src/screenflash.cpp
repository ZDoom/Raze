#include "ns.h"
#include "wh.h"

BEGIN_WH_NS

static int redcount, whitecount, greencount, bluecount;

void updatepaletteshifts() 
{
	if (whitecount != 0)
		whitecount = std::max(whitecount - TICSPERFRAME, 0);

	if (redcount != 0)
		redcount = std::max(redcount - TICSPERFRAME, 0);

	if (bluecount != 0)
		bluecount = std::max(bluecount - TICSPERFRAME, 0);

	if (greencount != 0)
		greencount = std::max(greencount - TICSPERFRAME, 0);
}

void startredflash(int damage) 
{
	redcount = std::min(redcount + 3 * damage, 100);
}

void startblueflash(int bluetime) 
{
	bluecount = std::min(bluecount + 3 * bluetime, 100);
}

void startwhiteflash(int whitetime) 
{
	whitecount = std::min(whitecount + 3 * whitetime, 100);
}

void startgreenflash(int greentime) 
{
	greencount = std::min(greencount + 3 * greentime, 100);
}

void resetflash() 
{
	redcount = 0;
	whitecount = 0;
	greencount = 0;
	bluecount = 0;
	videoTintBlood(0, 0, 0);
}

void applyflash()
{
	// no idea if this is righz. Needs to be tested.
	if (redcount) videoTintBlood(5*redcount/2, -5*redcount/2, -5*redcount/2);
	else if (greencount) videoTintBlood(-5*redcount/2, 5*redcount/2, -5*redcount/2);
	else if (bluecount) videoTintBlood(-5*redcount/2, -5*redcount/2, 5*redcount/2);
	else if (whitecount) videoTintBlood(5*redcount/2, 5*redcount/2, 5*redcount/2);
	else videoTintBlood(0, 0, 0);
}

END_WH_NS
