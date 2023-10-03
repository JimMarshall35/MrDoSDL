#pragma once 
#include "VectorTypes.h"

namespace CollisionHelpers {
	/*
	   Computes the direction of the three given points
	   Returns a positive value if they form a counter-clockwise orientation,
	   a negative value if they form a clockwise orientation,
	   and zero if they are collinear
	*/
	int direction(const vec2& p, const vec2& q, const vec2& r);

	// Checks if two line segments are collinear and overlapping
	bool areCollinearAndOverlapping(const vec2& a1, const vec2& b1, const vec2& a2, const vec2& b2);

	// Checks if two line segments intersect or not
	bool isintersect(const vec2& a1, const vec2& b1, const vec2& a2, const vec2& b2);

	bool CircleRect(float cx, float cy, float radius, float rx, float ry, float rw, float rh);

}