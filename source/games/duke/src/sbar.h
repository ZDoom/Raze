#pragma once

#include <array>
#include "statusbar.h"
#include "duke3d.h"

BEGIN_DUKE_NS


class DDukeCommonStatusBar : public DBaseStatusBar
{
	DECLARE_ABSTRACT_CLASS(DDukeCommonStatusBar, DBaseStatusBar)
	HAS_OBJECT_POINTERS

protected:
	TObjPtr<DHUDFont*> numberFont;
	TObjPtr<DHUDFont*> indexFont;
	TObjPtr<DHUDFont*> miniFont;
	TObjPtr<DHUDFont*> digiFont;
	double scale = 1;
	std::array<int, MAX_WEAPONS> ammo_sprites;
	std::array<int, 8> item_icons;

	DDukeCommonStatusBar();
	std::pair<const char*, EColorRange> ontext(struct player_struct *p);
	void DrawInventory(const struct player_struct* p, double x, double y, int align);
	PalEntry LightForShade(int shade);
public:
	void PrintLevelStats(int bottomy);

};

void DrawBorder();

END_DUKE_NS
