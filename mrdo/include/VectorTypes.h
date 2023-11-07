#pragma once
#include "CommonTypedefs.h"
#include <math.h>       /* sqrt */
# define M_PI           3.14159265358979323846  /* pi */

struct vec2
{
	float x;
	float y;

	vec2 operator-(const vec2& other) const
	{
		return { x - other.x, y - other.y };
	}

	vec2 operator+(const vec2& other) const
	{
		return { x + other.x, y + other.y };
	}

	vec2 operator*(const float other) const
	{
		return { x * other, y * other };
	}


	void operator+=(const vec2& other)
	{
		x += other.x;
		y += other.y;
	}

	bool operator==(const vec2& other) const
	{
		return (x == other.x) && (y == other.y);
	}
	bool operator!=(const vec2& other) const
	{
		return !((x == other.x) && (y == other.y));
	}

	float Magnitude() const
	{
		return sqrt(x * x + y * y);
	}

	vec2 Normalized() const
	{
		float mag = Magnitude();
		return { x / mag, y / mag };
	}

	vec2 Rotated(float degrees) const
	{
		vec2 normalised = Normalized();
		float x = normalised.x;
		float y = normalised.y;
		const float degreesToRadians = M_PI / 180.0;
		float angle = degrees * degreesToRadians;
		float _cos = cos(angle);
		float _sin = sin(angle);
		return
		{
			x*_cos-y*_sin,
			x*_sin+y*_cos
		};
	}
};

struct ivec2
{
	i32 x;
	i32 y;
	ivec2()
		:x(0),y(0)
	{}

	ivec2(i32 x, i32 y)
		:x(x), y(y)
	{}

	ivec2(u32 x, u32 y)
		:x((i32)x), y((i32)y)
	{}

	ivec2(float x, float y)
		:x((i32)x), y((i32)y)
	{}

	ivec2 operator+(const ivec2& other) const
	{
		return { x + other.x, y + other.y };
	}
	bool operator==(const ivec2& other) const
	{
		return (x == other.x) && (y == other.y);
	}
	bool operator!=(const ivec2& other) const
	{
		return !((x == other.x) && (y == other.y));
	}
};