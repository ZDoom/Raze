#pragma once

struct vec2_16_t
{
    int16_t x, y;
};


#if 0
struct vec2_t 
{
    int32_t x, y;
};

struct vec2u_t
{
    uint32_t x, y;
};

struct vec2f_t
{
    float x, y;
};

struct vec2d_t
{
    double x, y;
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