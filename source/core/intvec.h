#pragma once

class FSerializer;

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
	vec2_t& operator/= (int other) { x /= other; y /= other; return *this; }
    bool operator == (const vec2_t& other) const { return x == other.x && y == other.y; };
};
inline vec2_t operator/ (const vec2_t& vec, int other) { return { vec.x / other, vec.y / other }; }

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

    vec3_t() = default;
    vec3_t(const vec3_t&) = default;
    vec3_t(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
    vec3_t operator+(const vec3_t& other) const { return { x + other.x, y + other.y, z + other.z }; }
    vec3_t operator-(const vec3_t& other) const { return { x - other.x, y - other.y, z - other.z }; }
    vec3_t& operator+=(const vec3_t & other) { x += other.x; y += other.y; z += other.z; return *this; };
    vec3_t& operator-=(const vec3_t & other) { x -= other.x; y -= other.y; z += other.z; return *this; };

};




#if 0
struct vec2f_t
{
    float x, y;
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

FSerializer& Serialize(FSerializer& arc, const char* key, vec2_t& c, vec2_t* def);
FSerializer& Serialize(FSerializer& arc, const char* key, vec3_t& c, vec3_t* def);
