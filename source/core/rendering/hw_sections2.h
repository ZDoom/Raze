#include "hw_sections.h"

extern TArray<Section*> sections2;
extern TArrayView<TArrayView<Section*>> sections2PerSector;

void hw_CreateSections2();
using Outline = TArray<TArray<vec2_t>>;
using Point = std::pair<float, float>;
using FOutline = std::vector<std::vector<Point>> ; // Data type was chosen so it can be passed directly into Earcut.
Outline BuildOutline(Section* section);
