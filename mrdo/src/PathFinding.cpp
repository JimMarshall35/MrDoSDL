#include "PathFinding.h"
#include <vector>
#include <limits>

namespace PathFinding
{
	struct AStarNode
	{
		float GlobalScore = std::numeric_limits<float>::infinity();;
		float LocalScore = std::numeric_limits<float>::infinity();
		bool bVisited = false;
		AStarNode* Parent = nullptr;
	};
	static std::vector<AStarNode> NodesMap;

	void ResetNodeMapState()
	{
		for (AStarNode& node : NodesMap)
		{
			node.GlobalScore = std::numeric_limits<float>::infinity();
			node.LocalScore = std::numeric_limits<float>::infinity();
			node.bVisited = false;
			node.Parent = nullptr;
		}
	}

	void ResizeNodeMap(u32 newWidth, u32 newHeight)
	{
		NodesMap.resize(newWidth * newHeight);
		ResetNodeMapState();
	}
}