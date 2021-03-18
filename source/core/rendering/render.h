#pragma once
#include "build.h"

void render_drawrooms(vec3_t& position, int sectnum, fixed_t q16angle, fixed_t q16horizon, float rollang, bool mirror, bool planemirror);
void render_drawrooms_(vec3_t& position, int sectnum, fixed_t q16angle, fixed_t q16horizon, float rollang, float fov, bool mirror, bool planemirror);

