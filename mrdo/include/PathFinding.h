#pragma once
#include "CommonTypedefs.h"
struct uvec2;
class TiledWorld;

namespace PathFinding
{
	void ResizeNodeMap(u32 newWidth, u32 newHeight);

	void DoAstar(const uvec2& start, const uvec2& finish, uvec2* outPath, u32& outPathBufferSize, u32 pathBufferMaxSize, const TiledWorld* tiledWorld);
}