#include "CollisionHelpers.h"
#include <math.h>
#include <algorithm>

namespace CollisionHelpers
{
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
}