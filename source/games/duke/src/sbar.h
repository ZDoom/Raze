#pragma once

#include <array>
#include "statusbar.h"
#include "duke3d.h"

BEGIN_DUKE_NS


class DDukeCommonStatusBar : public DBaseStatusBar
{
protected:
    DHUDFont numberFont;
    DHUDFont indexFont;
    DHUDFont miniFont;
    DHUDFont digiFont;
    double scale = 1;
    std::array<int, MAX_WEAPONS> ammo_sprites;
    std::array<int, 8> item_icons;

	DDukeCommonStatusBar();
    std::pair<const char*, EColorRange> ontext(DukePlayer_t *p);
    void DrawInventory(const DukePlayer_t* p, double x, double y, int align);
    PalEntry LightForShade(int shade);
};

void drawstatusbar(int snum);

END_DUKE_NS
