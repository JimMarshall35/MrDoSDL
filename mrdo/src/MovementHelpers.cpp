#include "MovementHelpers.h"
#include <cassert>


const vec2 MovementHelpers::Up = { 0,-1 };
const vec2 MovementHelpers::Down = { 0,1 };
const vec2 MovementHelpers::Left = { -1,0 };
const vec2 MovementHelpers::Right = { 1,0 };

vec2 MovementHelpers::GetDirectionVector(MovementDirection direction)
{
	static const vec2 LUT[4] = { Up,Right,Down,Left };
	u32 i = (u32)direction;
	assert(i < 4);
	return LUT[i];
}