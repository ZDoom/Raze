#pragma once

int FindDistance2D(int x, int y);
double fFindDistance2D(int x, int y);
int FindDistance3D(int x, int y, int z);

inline int FindDistance3D(const vec3_t& vec)
{
	return FindDistance3D(vec.X, vec.Y, vec.Z);
}

inline int FindDistance2D(const vec2_t& vec)
{
	return FindDistance2D(vec.X, vec.Y);
}