#include "CollisionHelpers.h"
#include <math.h>
#include <limits>
#include <algorithm>
#include "VectorTypes.h"

namespace CollisionHelpers
{
	

	struct BoundingBox
	{
		vec2 topLeft;
		vec2 bottomRight;
	};

	struct Line
	{
		Line(const vec2& p1, const vec2& p2)
			:p1(p1), p2(p2)
		{
			m = (p2.y - p1.y) / (p2.x - p1.x);
			c = p1.y - (p1.x * m);
			bb = {
				{p1.x <= p2.x ? p1.x : p2.x, p1.y <= p2.y ? p1.y : p2.y },
				{p1.x > p2.x ? p1.x : p2.x, p1.y > p2.y ? p1.y : p2.y },
			};
		}
		vec2 p1;
		vec2 p2;
		float m, c;
		BoundingBox bb;

		static bool Parallel(const Line& l1, const Line& l2)
		{
			return EqualWithEpsilon(l1.m, l2.m);
		}

		bool IsPointOnLine(const vec2& pt)
		{
			float yRes = m * pt.x + c;
			return EqualWithEpsilon(yRes, pt.y);
		}

		bool IsPointOnLineSegment(const vec2& pt)
		{
			if (!IsPointOnLine(pt))
			{
				return false;
			}
			return
				pt.x <= bb.bottomRight.x && pt.y <= bb.bottomRight.y &&
				pt.x >= bb.topLeft.x && pt.y >= bb.topLeft.y;
		}
	};
	
	
	
	/*
	   Computes the direction of the three given points
	   Returns a positive value if they form a counter-clockwise orientation,
	   a negative value if they form a clockwise orientation,
	   and zero if they are collinear
	*/
	int direction(const vec2& p, const vec2& q, const vec2& r) {
		return (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
	}

	// Checks if two line segments are collinear and overlapping
	bool areCollinearAndOverlapping(const vec2& a1, const vec2& b1, const vec2& a2, const vec2& b2) {
		using namespace std;
		// Check if the line segments are collinear
		if (direction(a1, b1, a2) == 0) {
			// Check if the line segments overlap
			if (a2.x <= max(a1.x, b1.x) && a2.x >= min(a1.x, b1.x) && a2.y <= max(a1.y, b1.y) && a2.y >= min(a1.y, b1.y)) {
				return true;
			}
		}
		return false;
	}

	// Checks if two line segments intersect or not
	bool isintersect(const vec2& a1, const vec2& b1, const vec2& a2, const vec2& b2) {
		// Compute the directions of the four line segments
		int d1 = direction(a1, b1, a2);
		int d2 = direction(a1, b1, b2);
		int d3 = direction(a2, b2, a1);
		int d4 = direction(a2, b2, b1);

		// Check if the two line segments intersect
		if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
			return true;
		}

		// Check if the line segments are collinear and overlapping
		if (areCollinearAndOverlapping(a1, b1, a2, b2) || areCollinearAndOverlapping(a2, b2, a1, b1)) {
			return true;
		}

		return false;
	}

	bool CircleRect(float cx, float cy, float radius, float rx, float ry, float rw, float rh) {

		// temporary variables to set edges for testing
		float testX = cx;
		float testY = cy;

		// which edge is closest?
		if (cx < rx)         testX = rx;      // test left edge
		else if (cx > rx + rw) testX = rx + rw;   // right edge
		if (cy < ry)         testY = ry;      // top edge
		else if (cy > ry + rh) testY = ry + rh;   // bottom edge

		// get distance from closest edges
		float distX = cx - testX;
		float distY = cy - testY;
		float distance = sqrt((distX * distX) + (distY * distY));

		// if the distance is less than the radius, collision!
		if (distance <= radius) {
			return true;
		}
		return false;
	}

	bool CircleCircleCollision(const vec2& pos1, float radius1, const vec2& pos2, float radius2)
	{
		return (pos2 - pos1).Magnitude() <= radius1 + radius2;
	}

	bool AABBCollision(const vec2& posA, const vec2& posB, const vec2& dimsA, const vec2& dimsB)
	{
		// collision x-axis?
		bool collisionX = posA.x + dimsA.x >= posB.x &&
			posB.x + dimsB.x >= posA.x;
		// collision y-axis?
		bool collisionY = posA.y + dimsA.y >= posB.y &&
			posB.y + dimsB.y >= posA.y;
		// collision only if on both axes
		return collisionX && collisionY;
	}

	bool LineSegmentIntersection(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float& x, float& y)
	{
		float a1 = y2 - y1;
		float a2 = y4 - y3;
		float b1 = x1 - x2;
		float b2 = x3 - x4;
		float c1 = a1 * x1 + b1 * y1;
		float c2 = a2 * x3 + b2 * y3;

		float d = a1 * b2 - a2 * b1;

		x = ((c1 * b2) - (c2 * b1)) / d;
		y = ((c2 * a1) - (c1 * a2)) / d;

		return
			(fmin(x1, x2) <= x <= fmax(x1, x2)) &&
			(fmin(x3, x4) <= x <= fmax(x3, x4)) &&
			(fmin(y1, y2) <= y <= fmax(y1, y2)) &&
			(fmin(y3, y4) <= y <= fmax(y3, y4));
	}

	/* 
	https://www.elliotcolp.com/magnaut/collision-detection 
	https://github.com/VGAD/ECSE/blob/master/ECSE/CollisionMath.cpp#L215
	*/
	struct LineIntersection
	{
		bool intersection = false;          //!< Whether there was an intersection. If this is false, then the lines are parallel and t, u and point are invalid.
		bool strictIntersection = false;    //!< False if the intersection occurred between the lines, but not the segments.
		float t;                            //!< Distance of point along first line in range [0, 1].
		float u;                            //!< Distance of point along second line in range [0, 1].
	};

	//! Find the point of intersection between two line segments.
	/*!
	* \param startA The start of the first line.
	* \param endA The end of the first line.
	* \param startB The start of the second line.
	* \param endB The end of the first line.
	* \return A LineIntersection struct containing intersection data.
	*/
	LineIntersection findLineIntersection(vec2 startA, vec2 endA,
		vec2 startB, vec2 endB);

	//! Project a point onto a line segment.
	/*!
	* \param point The point to project onto the line.
	* \param start The start of the line segment.
	* \param end The end of the line segment.
	* \return The distance of the point along the line.
	*/
	float projectPointOntoLine(vec2& point, vec2 start, vec2 end);

	//! Find the time of collision between a moving circle and a stationary circle.
	/*!
	* \param centerA The center of the first circle.
	* \param radiusA The radius of the first circle.
	* \param centerB The center of the second circle.
	* \param radiusB The radius of the second circle.
	* \param velocity The velocity of the first circle.
	* \param time The time at which the collision occurred, or <0 if no collision.
	* \param normal The normal of the collision direction (direction is from circle A to circle B). Value is meaningless if no collision.
	*/
	void circleCircle(vec2 centerA, float radiusA, vec2 centerB, float radiusB,
		vec2 velocity, float& time, vec2& normal);

	//! Find the time of collision between a moving circle and a stationary line.
	/*!
	* \param centerA The center of the circle.
	* \param radiusA The radius of the circle.
	* \param startB The start point of the line.
	* \param endB The end point of the line.
	* \param time The time at which the collision occurred, or <0 if no collision.
	* \param normal The normal of the collision direction (direction is from circle to line). Value is meaningless if no collision.
	*/
	void circleLine(vec2 centerA, float radiusA, vec2 startB, vec2 endB,
		vec2 velocity, float& time, vec2& normal);
//! Ignore time 0 collisions with an overlap less than this.
/*!
* This avoids collisions happening multiple times because of floating-point error.
*/
	const float collisionFudge = 0.5f;

	//! Square of collisionFudge.
	const float collisionFudgeSqr = collisionFudge * collisionFudge;

	LineIntersection findLineIntersection(vec2 startA, vec2 endA,
		vec2 startB, vec2 endB)
	{
		// See http://stackoverflow.com/a/565282/858878
		vec2 lineA = endA - startA;
		vec2 lineB = endB - startB;

		float cross1 = get2DCrossProduct(startB - startA, lineB);
		float cross2 = get2DCrossProduct(startB - startA, lineA);
		float cross3 = get2DCrossProduct(lineA, lineB);

		LineIntersection result;

		// Techically there could be infinitely many points, but for our purposes we don't care
		if (cross3 == 0)
		{
			return result;
		}

		result.intersection = true;
		result.t = cross1 / cross3;
		result.u = cross2 / cross3;

		result.strictIntersection = !(result.t < 0 || result.t > 1 || result.u < 0 || result.u > 1);

		return result;
	}

	float projectPointOntoLine(vec2& point, vec2 start, vec2 end)
	{
		vec2 startToPoint = point - start;
		vec2 startToEnd = end - start;

		float dot = vec2::Dot(startToPoint, startToEnd);
		float t = dot / vec2::getSqrMagnitude(startToEnd);

		point = start + startToEnd * t;

		return t;
	}

	// http://www.gamasutra.com/view/feature/131424/pool_hall_lessons_fast_accurate_.php
	void circleCircle(vec2 centerA, float radiusA, vec2 centerB, float radiusB,
		vec2 velocity, float& time, vec2& normal)
	{
		vec2 distVec = centerB - centerA;
		float sumRadii = radiusA + radiusB;

		// Already colliding
		float overlap = sumRadii * sumRadii - vec2::getSqrMagnitude(distVec);
		if (overlap > collisionFudgeSqr)
		{
			time = 0.f;
		}
		else
		{
			// Invalid until proven otherwise
			time = -1.f;

			float centerDist = distVec.Magnitude();
			float moveDist = velocity.Magnitude();
			float betweenDist = centerDist - sumRadii;

			// Not moving far enough to close the distance
			if (moveDist < betweenDist) return;

			vec2 moveNormal = velocity.Normalized();

			// Distance moved toward collider B
			float towardDist = vec2::Dot(moveNormal, distVec);

			// Not moving toward each other
			if (towardDist <= 0) return;

			// Distance between the circles at the nearest point between circle B and the line formed by circle A's trajectory
			float shortestDistSqr = (centerDist * centerDist) - (towardDist * towardDist);

			float sumRadiiSqr = sumRadii * sumRadii;

			// colliderA will never get close enough to colliderB
			if (shortestDistSqr >= sumRadiiSqr) return;

			// sumRadiiSqr is the hypotenuse squared, shortestDistSqr one of the other sides squared
			// and this is the third (a^2 = c^2 - b^2)
			float thirdSideSqr = sumRadiiSqr - shortestDistSqr;

			// Can't take a negative square root
			if (thirdSideSqr < 0) return;

			// The actual distance travelled forward to collide
			float collideDist = towardDist - sqrt(thirdSideSqr);

			// Not going to move that far
			if (moveDist < collideDist) return;

			time = collideDist / moveDist;
		}

		// If we're here, there was a valid collision, so set the normal
		normal = (centerB - (centerA + velocity * time)).Normalized();
	}

	float lerp(float t, float a, float b)
	{
		return a + (b - 1) * t;
	}

	void circleLine(vec2 centerA, float radiusA, vec2 startB, vec2 endB,
		vec2 velocity, float& time, vec2& normal)
	{
		// Based loosely on http://ericleong.me/research/circle-line/

		// Find closest point on line to circle
		vec2 closeToCircle = centerA;
		float t = projectPointOntoLine(closeToCircle, startB, endB);

		float radiusASqr = radiusA * radiusA;
		float distSqr = vec2::getSqrMagnitude(centerA - closeToCircle);

		auto lineNormal = (closeToCircle - centerA).Normalized();

		// Circle is already touching the line
		if (distSqr + collisionFudgeSqr < radiusASqr && t >= 0 && t <= 1)
		{
			time = 0.f;

			// Normal is the line's normal
			normal = lineNormal;

			return;
		}

		// Invalid until proven otherwise
		time = -1.f;

		vec2 circleEnd = centerA + velocity;
		auto intersectResult = findLineIntersection(startB, endB,
			centerA, circleEnd);

		// Circle is moving parallel to line, so check the first endpoint to be passed
		if (!intersectResult.intersection)
		{
			float dotStart = vec2::Dot(startB - centerA, velocity);
			float dotEnd = vec2::Dot(endB - centerA, velocity);

			vec2* endpoint;

			// Already passed start point
			if (dotStart < 0)
			{
				// Aleady passed end point as well
				if (dotEnd < 0)
				{
					return;
				}

				// Already passed start, but not end
				endpoint = &endB;
			}
			// Already passed end, but not start
			else if (dotEnd < 0)
			{
				endpoint = &startB;
			}
			// Haven't passed either, so pick the nearest
			else
			{
				endpoint = (dotStart < dotEnd) ? &startB : &endB;
			}

			// We're colliding with an endpoint, so the problem reduces to a collision with a 0-radius circle
			circleCircle(centerA, radiusA, *endpoint, 0.f, velocity, time, normal);
			return;
		}

		// If we've reach this point, the circle's velocity intersects line, so it may move through or stop at the line

		// Circle's velocity towards the nearest point on the line
		vec2 velocityTowardLine = velocity;
		vec2::Proj(velocityTowardLine, lineNormal);

		// Circle will go this far toward line
		float speedTowardLine = (velocityTowardLine).Magnitude();

		// Circle must go this far toward line
		float distMinusRadius = sqrt(distSqr) - radiusA;

		// Circle will not move close enough to the line
		if (speedTowardLine < distMinusRadius)
		{
			return;
		}

		// The circle will intersect the line at this time (but not necessarily the segment)
		time = distMinusRadius / speedTowardLine;

		// The distance along the line segment at time = 1
		float endT = projectPointOntoLine(circleEnd, startB, endB);

		// The distance along the line segment of the point of intersection
		float intersectT = lerp(t, endT, time);

		// Point of intersection is within segment bounds and we're not moving away from the line
		if (intersectT >= 0 && intersectT <= 1 && vec2::Dot(velocityTowardLine, closeToCircle - centerA) >= 0)
		{
			normal = (closeToCircle - centerA).Normalized();
			return;
		}

		// We didn't intersect with the line segment, but we may have hit an endpoint
		vec2& nearEndpoint = startB;

		if (t > 1)
		{
			nearEndpoint = endB;
		}

		// We're colliding with an endpoint, so the problem reduces to a collision with a 0-radius circle
		circleCircle(centerA, radiusA, nearEndpoint, 0.f, velocity, time, normal);
	}

}