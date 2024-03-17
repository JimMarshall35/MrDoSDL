#pragma once
#include "CommonTypedefs.h"
#include <math.h>       /* sqrt */
# define M_PI           3.14159265358979323846  /* pi */
#include <stdint.h>


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

	float MagnitudeSqr() const
	{
		return (x * x + y * y);
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
			x * _cos - y * _sin,
			x * _sin + y * _cos
		};
	}

	static float Dot(const vec2& l, const vec2& r)
	{
		return l.x * r.x + l.y * r.y;
	}
	static float getSqrMagnitude(const vec2& v)
	{
		return v.x * v.x + v.y * v.y;
	}
	// project u onto v
	static vec2 Proj(const vec2& u, const vec2& v)
	{
		return v * (Dot(u, v) / Dot(v, v));
	}

	static float projectPointOntoLine(vec2& point, vec2 start, vec2 end)
	{
		vec2 startToPoint = point - start;
		vec2 startToEnd = end - start;

		float dot = Dot(startToPoint, startToEnd);
		float t = dot / getSqrMagnitude(startToEnd);

		point = start + startToEnd * t;

		return t;
	}

	static vec2 Reflect(const vec2& direction, const vec2& normal)
	{
		return direction - normal * 2 * (vec2::Dot(direction, normal));
	}
};
inline float get2DCrossProduct(const vec2& v1, const vec2& v2)
{
	return v1.x * v2.y - v1.y * v2.x;
}
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

	ivec2(i64 x, i64 y)
		:x((i32)x), y((i64)y)
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