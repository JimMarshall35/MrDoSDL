#pragma once


enum class MovementDirection
{
	Up = 0,
	Right,
	Down,
	Left,
	Undefined,
	MAX
};

enum class CollidingCellRelationship
{
	Above,
	Below,
	Left,
	Right,
	NotColliding,
	Undefined
};