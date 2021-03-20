#pragma once

struct vec2_16_t
{
    int16_t x, y;
};

struct vec2_t
{
    int32_t x, y;

    vec2_t() = default;
    vec2_t(const vec2_t&) = default;
    vec2_t(int x_, int y_) : x(x_), y(y_) {}
    vec2_t operator+(const vec2_t& other) const { return { x + other.x, y + other.y }; }
    vec2_t operator-(const vec2_t& other) const { return { x - other.x, y - other.y }; }
    vec2_t& operator+=(const vec2_t& other) { x += other.x; y += other.y; return *this; };
    vec2_t& operator-=(const vec2_t& other) { x -= other.x; y -= other.y; return *this; };
};



#if 0
struct vec2f_t
{
    float x, y;
};

struct vec3_t
{
    union 
	{
        struct 
		{ 
			int32_t x, y, z; 
		};
        vec2_t  vec2;
    };
};

struct vec3_16_t
{
    union 
	{
        struct 
		{ 
			int16_t x, y, z; 
		};
        vec2_16_t vec2;
    };
};
#endif
