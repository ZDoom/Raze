#include "hw_sections.h"
struct Section2;

enum ESEctionFlag
{
	Unclosed = 1,	// at least one unclosed loop
	Dumped = 2,		// builder was unable to properly construct, so content may not be usable for triangulator.
	BadWinding = 4,
};

struct Section2Loop
{
	TArrayView<int> walls;
};

struct Section2
{
	uint8_t flags;
	uint8_t dirty;
	uint8_t geomflags;
	unsigned index;
	int sector;
	// this uses a memory arena for storage, so use TArrayView instead of TArray
	TArrayView<int> lines;
	TArrayView<Section2Loop> loops;
};

extern TArray<Section2*> sections2;
extern TArrayView<TArrayView<Section2*>> sections2PerSector;

void hw_CreateSections2();
using Outline = TArray<TArray<vec2_t>>;
using Point = std::pair<float, float>;
using FOutline = std::vector<std::vector<Point>> ; // Data type was chosen so it can be passed directly into Earcut.
Outline BuildOutline(Section2* section);
