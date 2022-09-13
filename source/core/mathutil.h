#pragma once

int FindDistance2D(int x, int y);
double fFindDistance2D(int x, int y);

inline int FindDistance2D(const vec2_t& vec)
{
	return FindDistance2D(vec.X, vec.Y);
}
