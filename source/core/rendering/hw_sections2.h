
struct Section2;

struct Section2Wall
{
	// references to game data
	vec3_t* v1;					// points to start vertex in wall[]
	vec3_t* v2; 				// points to end vertex in wall[]
	walltype* wall;				// points to the actual wall this belongs to - this is NOT necessarily the same as v1 and can be null.
	
	// references to section data
	Section2Wall* backside;		// points to this wall's back side
	Section2* frontsection;
	Section2* backsection;		// if this is null the wall is one-sided
};

struct Loop
{
	TArrayView<Section2Wall*> walls;
	bool unclosed;
};

struct Section2
{
	sectortype* sector;
	// this uses a memory arena for storage, so use TArrayView instead of TArray
	TArrayView<Section2Wall*> walls;
	TArrayView<Loop*> loops;		
};

void hw_CreateSections2();