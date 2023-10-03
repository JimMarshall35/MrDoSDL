#pragma once
#include "CommonTypedefs.h"
#include <math.h>       /* sqrt */

struct vec2
{
	float x;
	float y;
	vec2 operator+(const vec2& other)
	{
		return { x + other.x, y + other.y };
	}

	vec2 operator*(const float other)
	{
		return { x * other, y * other };
	}


	void operator+=(const vec2& other)
	{
		x += other.x;
		y += other.y;
	}

	bool operator==(const vec2& other)
	{
		return (x == other.x) && (y == other.y);
	}
	bool operator!=(const vec2& other)
	{
		return !((x == other.x) && (y == other.y));
	}

	float Magnitude()
	{
		return sqrt(x * x + y * y);
	}

	vec2 Normalized()
	{
		float mag = Magnitude();
		return { x / mag, y / mag };
	}
};

struct ivec2
{
	i32 x;
	i32 y;
	ivec2 operator+(const ivec2& other)
	{
		return { x + other.x, y + other.y };
	}
	bool operator==(const ivec2& other)
	{
		return (x == other.x) && (y == other.y);
	}
	bool operator!=(const ivec2& other)
	{
		return !((x == other.x) && (y == other.y));
	}
};

struct uvec2
{
	u32 x;
	u32 y;
	uvec2 operator+(const uvec2& other)
	{
		return { x + other.x, y + other.y };
	}
	bool operator==(const uvec2& other)
	{
		return (x == other.x) && (y == other.y);
	}
	bool operator!=(const uvec2& other)
	{
		return !((x == other.x) && (y == other.y));
	}
};