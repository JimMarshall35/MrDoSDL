#pragma once
#include "CommonTypedefs.h"
enum class LevelSource
{
	Undefined,
	ArcadeLevels,
	MapMaker
};

struct LevelLoadData
{
	LevelSource Source;
	i32 LevelIndex;
};