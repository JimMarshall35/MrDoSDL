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

	bool AABBCollision(const vec2& posA, const vec2& posB, const vec2& dimsA, const vec2& dimsB);


	//vec2 SweptCircleLineSegment(const vec2& circle, float radius, const vec2& velocity, const vec2& l1, const vec2& l2, vec2& outNormal);
	void circleLine(vec2 centerA, float radiusA, vec2 startB, vec2 endB,
		vec2 velocity, float& time, vec2& normal);

	template<typename Tfloat>
	bool EqualWithEpsilon(Tfloat f1, Tfloat f2)
	{
		return fabs(f1 - f2) < 1e-5;//std::numeric_limits<Tfloat>::epsilon;
	}

	template<typename Tvec>
	void CalcBoundingBox(Tvec& outTL, Tvec& outBR, const Tvec* points, int numPoints)
	{
		outTL = { 9999999,9999999 };//{ std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity() };
		outBR = { std::numeric_limits<float>::min(),std::numeric_limits<float>::min() };
		for (int i = 0; i < numPoints; i++)
		{
			const Tvec& p = points[i];
			if (p.x < outTL.x)
			{
				outTL.x = p.x;
			}
			if (p.y < outTL.y)
			{
				outTL.y = p.y;
			}
			if (p.x > outBR.x)
			{
				outBR.x = p.x;
			}
			if (p.y > outBR.y)
			{
				outBR.y = p.y;
			}
		}
	}
}