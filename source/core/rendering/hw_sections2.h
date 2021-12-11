
struct Section2;

enum ESEctionFlag
{
	Unclosed = 1,	// at least one unclosed loop
	Dumped = 2,		// builder was unable to properly construct, so content may not be usable for triangulator.
	BadWinding = 4,
};

struct Section2Wall
{
	// references to game data
	vec2_t* v1;					// points to start vertex in wall[]
	vec2_t* v2; 				// points to end vertex in wall[]
	walltype* wall;				// points to the actual wall this belongs to - this is NOT necessarily the same as v1 and can be null.
	
	// references to section data
	Section2Wall* backside;		// points to this wall's back side
	Section2* frontsection;
	Section2* backsection;		// if this is null the wall is one-sided
};

struct Section2Loop
{
	TArrayView<Section2Wall*> walls;
};

struct Section2
{
	int flags;
	unsigned index;
	sectortype* sector;
	// this uses a memory arena for storage, so use TArrayView instead of TArray
	TArrayView<Section2Wall*> walls;
	TArrayView<Section2Loop> loops;
};

extern TArray<Section2*> sections2;
extern TArrayView<TArrayView<Section2*>> sections2PerSector;

void hw_CreateSections2();
using Outline = TArray<TArray<vec2_t>>;
using Point = std::pair<float, float>;
using FOutline = std::vector<std::vector<Point>> ; // Data type was chosen so it can be passed directly into Earcut.
Outline BuildOutline(Section2* section);
