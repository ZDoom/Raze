
struct Section2;

enum ESEctionFlag
{
	Unclosed = 1,	// at least one unclosed loop
	Dumped = 2,		// builder was unable to properly construct, so content may not be usable for triangulator.
	BadWinding = 4,
};

struct Section2Wall
{
	int index;
	int section;
	int startpoint;
	int endpoint;
	int wall; // points to the actual wall this belongs to - this is NOT necessarily the same as startpoint and can be -1.
	Section2Wall* backside;		// this is better kept as pointer because of reindexing when splitting a section.

	vec2_t v1() const { return ::wall[startpoint].pos; }
	vec2_t v2() const { return ::wall[endpoint].pos; }
	walltype* wallp() const { return &::wall[wall]; }

};

struct Section2Loop
{
	TArrayView<Section2Wall*> walls;
};

struct Section2
{
	uint8_t flags;
	uint8_t dirty;
	uint8_t geomflags;
	unsigned index;
	sectortype* sector;
	// this uses a memory arena for storage, so use TArrayView instead of TArray
	TArrayView<Section2Wall*> walls;
	TArrayView<Section2Loop> loops;
};

extern TArray<Section2*> sections2;
extern TArrayView<TArrayView<Section2*>> sections2PerSector;
extern TArray<Section2Wall*> section2walls;

void hw_CreateSections2();
using Outline = TArray<TArray<vec2_t>>;
using Point = std::pair<float, float>;
using FOutline = std::vector<std::vector<Point>> ; // Data type was chosen so it can be passed directly into Earcut.
Outline BuildOutline(Section2* section);
