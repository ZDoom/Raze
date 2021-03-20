#pragma once
#include "build.h"

void render_drawrooms(spritetype* playersprite, const vec3_t& position, int sectnum, fixed_t q16angle, fixed_t q16horizon, float rollang, int flags);

enum ERenderSceneFlags
{
	RSF_MIRROR = 1,
	RSF_PLANEMIRROR = 2,
	RSF_UPDATESECTOR = 4,
};
