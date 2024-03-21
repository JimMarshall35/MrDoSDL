#include "PathFinding.h"
#include <vector>
#include <limits>
#include <cassert>
#include <algorithm>
#include <functional>
#include "VectorTypes.h"
#include <list>
#include "TiledWorld.h"
#include "ConfigFile.h"

namespace PathFinding
{
	struct AStarNode
	{
		float GlobalScore = std::numeric_limits<float>::infinity();;
		float LocalScore = std::numeric_limits<float>::infinity();
		bool bVisited = false;
		AStarNode* Parent = nullptr;
		ivec2 GridCoords;
	};
	static std::vector<AStarNode> NodesMap;

	static u32 NodesMapWidth = 0;

	static u32 NodesMapHeight = 0;

	static PtrMinHeap<AStarNode, float>* PriorityQueue = nullptr;

	void ResetNodeMapState()
	{
		assert(NodesMap.size() == NodesMapHeight * NodesMapWidth);
		for (u32 y = 0; y < NodesMapHeight; y++)
		{
			for (u32 x = 0; x < NodesMapWidth; x++)
			{
				AStarNode& node = NodesMap[y * NodesMapWidth + x];
				node.GlobalScore = std::numeric_limits<float>::infinity();
				node.LocalScore = std::numeric_limits<float>::infinity();
				node.bVisited = false;
				node.Parent = nullptr;
				node.GridCoords = { x,y };
			}
		}
	}

	AStarNode* FindNode(const ivec2& nodeCoords)
	{
		assert(NodesMapWidth * nodeCoords.y + nodeCoords.x < NodesMap.size());
		return &NodesMap[nodeCoords.y * NodesMapWidth + nodeCoords.x];
	}

	void ResizeNodeMap(u32 newWidth, u32 newHeight)
	{
		NodesMap.resize(newWidth * newHeight);
		NodesMapWidth = newWidth;
		NodesMapHeight = newHeight;
		ResetNodeMapState();
	}

	void Initialise(int priorityQueueSize)
	{
		assert(PriorityQueue == nullptr);
		PriorityQueue = new PtrMinHeap<AStarNode, float>(priorityQueueSize,
			[](const AStarNode& nodeA, const AStarNode& nodeB) { return nodeA.GlobalScore < nodeB.GlobalScore; });

	}

	void DeInit()
	{
		assert(PriorityQueue);
		delete PriorityQueue;
	}

	typedef std::function<void(AStarNode*)> NodeNeighbourIterator;
	typedef std::function<void(AStarNode*, const TiledWorld*, NodeNeighbourIterator)> NodeNeighboutIteratorApplier;

	void IterateNodeNeighbours(AStarNode* node, const TiledWorld* tiledWorld, NodeNeighbourIterator iterator)
	{
		ivec2 signedGridCoords = ivec2{ (i32)node->GridCoords.x, (i32)node->GridCoords.y };
		if (node->GridCoords.y >= 2) // todo: fix game so that tiledWorld is only the game tiles and not the hud at top and bottom hence why some of these are randomly 2 and not 1
		{
			if (!tiledWorld->IsBarrierBetween(signedGridCoords, signedGridCoords + ivec2{0, -1}))
			{
				AStarNode* neighbour = FindNode(ivec2{ node->GridCoords.x, node->GridCoords.y - 1 });
				iterator(neighbour);
			}
		}
		if (node->GridCoords.x < tiledWorld->GetActiveLevelWidth() - 1)
		{
			if (!tiledWorld->IsBarrierBetween(signedGridCoords, signedGridCoords + ivec2{ 1, 0 }))
			{
				AStarNode* neighbour = FindNode(node->GridCoords + ivec2{ 1, 0 });
				iterator(neighbour);
			}
		}
		if (node->GridCoords.y < tiledWorld->GetActiveLevelHeight() - 2)
		{
			if (!tiledWorld->IsBarrierBetween(signedGridCoords, signedGridCoords + ivec2{ 0, 1 }))
			{
				AStarNode* neighbour = FindNode(ivec2{node->GridCoords.x, node->GridCoords.y + 1});
				iterator(neighbour);
			}
		}
		if (node->GridCoords.x >= 1)
		{
			if (!tiledWorld->IsBarrierBetween(signedGridCoords, signedGridCoords + ivec2{ -1, 0 }))
			{
				AStarNode* neighbour = FindNode(ivec2{ node->GridCoords.x - 1, node->GridCoords.y });
				iterator(neighbour);
			}
		}
	}

	void DiggingEnemyIterateNeighbours(AStarNode* node, const TiledWorld* tiledWorld, NodeNeighbourIterator iterator, const ivec2& obstruction)
	{
		ivec2 signedGridCoords = ivec2{ (i32)node->GridCoords.x, (i32)node->GridCoords.y };
		if (node->GridCoords.y >= 2) // todo: fix game so that tiledWorld is only the game tiles and not the hud at top and bottom hence why some of these are randomly 2 and not 1
		{
			if (node->GridCoords + ivec2{ 0, -1 } != obstruction)
			{
				AStarNode* neighbour = FindNode(ivec2{ node->GridCoords.x, node->GridCoords.y - 1 });
				iterator(neighbour);
			}
		}
		if (node->GridCoords.x < tiledWorld->GetActiveLevelWidth() - 1)
		{
			if (node->GridCoords + ivec2{ 1, 0 } != obstruction)
			{
				AStarNode* neighbour = FindNode(node->GridCoords + ivec2{ 1, 0 });
				iterator(neighbour);
			}
		}
		if (node->GridCoords.y < tiledWorld->GetActiveLevelHeight() - 2)
		{
			if (node->GridCoords + ivec2{ 0, 1 } != obstruction)
			{
				AStarNode* neighbour = FindNode(ivec2{ node->GridCoords.x, node->GridCoords.y + 1 });
				iterator(neighbour);
			}
		}
		if (node->GridCoords.x >= 1)
		{
			if (node->GridCoords + ivec2{ -1, 0 } != obstruction)
			{
				AStarNode* neighbour = FindNode(ivec2{ node->GridCoords.x - 1, node->GridCoords.y });
				iterator(neighbour);
			}
		}
	}

	/// <summary>
	/// returns Path successfully found
	/// </summary>
	bool DoAStar(const ivec2& start, const ivec2& finish, ivec2* outPath, u32& outPathBufferSize, u32 pathBufferMaxSize, const TiledWorld* tiledWorld , const NodeNeighboutIteratorApplier& iterApplier)
	{
		// based on https://github.com/OneLoneCoder/Javidx9/blob/master/ConsoleGameEngine/SmallerProjects/OneLoneCoder_PathFinding_AStar.cpp
		// changed data structure used from list to min heap
		auto distance = [](const ivec2& a, const ivec2& b) // For convenience
		{
			return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
		};

		auto heuristic = [distance](const AStarNode* a, const AStarNode* b) // So we can experiment with heuristic
		{
			return distance(a->GridCoords, b->GridCoords);
		};
		ResetNodeMapState();
		AStarNode* startNode = FindNode(start);
		AStarNode* finishNode = FindNode(finish);
		startNode->LocalScore = 0;
		startNode->GlobalScore = heuristic(startNode, finishNode);
		outPathBufferSize = 0;
		assert(PriorityQueue->IsEmpty());
		PriorityQueue->Insert(startNode);

		AStarNode* currentNode = nullptr;

		while (!PriorityQueue->IsEmpty() && currentNode != finishNode)
		{
			// Front of listNotTestedNodes is potentially the lowest distance node. Our
			// list may also contain nodes that have been visited, so ditch these...
			while (!PriorityQueue->IsEmpty() && PriorityQueue->Front()->bVisited)
			{
				PriorityQueue->ExtractMin();
			}

			// ...or abort because there are no valid nodes left to test
			if (PriorityQueue->IsEmpty())
			{
				break;
			}
				

			currentNode = PriorityQueue->Front();
			currentNode->bVisited = true; // We only explore a node once


			// Check each of this node's neighbours...
			auto iterator = [finishNode, &heuristic, &currentNode](AStarNode* nodeNeighbour)
			{
				
				if (!nodeNeighbour->bVisited)
				{
					PriorityQueue->Insert(nodeNeighbour);
				}

				// Calculate the neighbours potential lowest parent distance
				float fPossiblyLowerGoal = currentNode->LocalScore + heuristic(currentNode, nodeNeighbour);

				// If choosing to path through this node is a lower distance than what 
				// the neighbour currently has set, update the neighbour to use this node
				// as the path source, and set its distance scores as necessary
				if (fPossiblyLowerGoal < nodeNeighbour->LocalScore)
				{
					nodeNeighbour->Parent = currentNode;
					nodeNeighbour->LocalScore = fPossiblyLowerGoal;

					// The best path length to the neighbour being tested has changed, so
					// update the neighbour's score. The heuristic is used to globally bias
					// the path algorithm, so it knows if its getting better or worse. At some
					// point the algo will realise this path is worse and abandon it, and then go
					// and search along the next best path.
					nodeNeighbour->GlobalScore = nodeNeighbour->GlobalScore + heuristic(nodeNeighbour, finishNode);
				}
			};

			iterApplier(currentNode, tiledWorld, iterator);
		}
		PriorityQueue->Clear();
		// write path to output buffer, by following the chain of parent pointers

		currentNode = finishNode;
		if (!finishNode)
		{
			return false;
		}
		while (currentNode != startNode && outPathBufferSize < pathBufferMaxSize)
		{
			outPath[outPathBufferSize++] = currentNode->GridCoords;
			currentNode = currentNode->Parent;
			if (!currentNode)
			{
				outPathBufferSize = 0;
				return false;
			}
		}
		return true;
	}
	bool DoAStarNormalEnemy(const ivec2& start, const ivec2& finish, ivec2* outPath, u32& outPathBufferSize, u32 pathBufferMaxSize, const TiledWorld* tiledWorld)
	{
		return DoAStar(start, finish, outPath, outPathBufferSize, pathBufferMaxSize, tiledWorld, NodeNeighboutIteratorApplier(&IterateNodeNeighbours));
	}
	bool DoDiggingEnemyAStar(const ivec2& start, ivec2& finish, ivec2* outPath, u32& outPathBufferSize, u32 pathBufferMaxSize, const TiledWorld* tiledWorld, const ivec2& obstruction)
	{
		return DoAStar(start, finish, outPath, outPathBufferSize, pathBufferMaxSize, tiledWorld,
			[&obstruction](AStarNode* node, const TiledWorld* tiledWorld, NodeNeighbourIterator iterator) {
			DiggingEnemyIterateNeighbours(node, tiledWorld, iterator, obstruction);
		});
	}
}