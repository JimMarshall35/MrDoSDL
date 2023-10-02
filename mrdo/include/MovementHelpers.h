#pragma once
#include "VectorTypes.h"
#include "MovementTypes.h"

namespace MovementHelpers
{
	vec2 GetDirectionVector(MovementDirection direction);
	extern const vec2 Up;
	extern const vec2 Down;
	extern const vec2 Left;
	extern const vec2 Right;
}