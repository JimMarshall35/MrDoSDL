#pragma once
#include "CommonTypedefs.h"
#include <functional>
#include <memory>
#include <cassert>
#include <iostream>

struct uvec2;
class TiledWorld;
class IConfigFile;

namespace PathFinding
{
	void ResizeNodeMap(u32 newWidth, u32 newHeight);

	void Initialise(int configFile);

	void DeInit();

	void DoAstar(const uvec2& start, const uvec2& finish, uvec2* outPath, u32& outPathBufferSize, u32 pathBufferMaxSize, const TiledWorld* tiledWorld);

	template<typename T, typename TKey>
	class PtrMinHeap
	{
		// based on https://www.codingninjas.com/studio/library/implement-min-heap-in-c
	public:
		static_assert(std::is_arithmetic_v<TKey> == true, "must be arithmetic");

		typedef std::function<bool(const T&, const T&)> KeyLessThan;

		PtrMinHeap(int capacity, const KeyLessThan& lessThanFunc)
		:LessThanFunc(lessThanFunc),
		Array(std::make_unique<T*[]>(capacity)),
		Capacity(capacity),
		Size(0){}

		// method to remove minimum element (or root) from min heap
		T* ExtractMin() {
			if (Size <= 0)
			{
				return nullptr;
			}
				
			if (Size == 1)
			{
				Size--;
				return Array[0];
			}

			// remove the minimum value from the heap.
			T* root = Array[0];
			Array[0] = Array[Size - 1];
			Size--;
			MinHeapify(0);

			return root;
		}

		// method to get index of parent of node at index i
		int Parent(int i) const { return (i - 1) / 2; }

		// method to get index of left child of node at index i
		int Left(int i) const { return (2 * i + 1); }

		// method to get index of right child of node at index i
		int Right(int i) const { return (2 * i + 2); }

		void Swap(T*& a, T*& b)
		{
			T* temp = a;
			a = b;
			b = temp;
		}

		// method to heapify a subtree with the root at given index i
		void MinHeapify(int i) {
			/* A recursive method to heapify 'heap_array' */
			int l = Left(i);
			int r = Right(i);

			int smallest = i;
			if (l < Size && LessThanFunc(*Array[l], *Array[i]))
			{
				smallest = l;
			}
				
			if (r < Size && LessThanFunc(*Array[r], *Array[smallest]))
			{
				smallest = r;
			}

			if (smallest != i) {
				Swap(Array[i], Array[smallest]);
				MinHeapify(smallest);
			}
		}

		// method to inserts a new key 'k'
		void Insert(T* k) {
			if (Size == Capacity) {
				std::cout << "\nOverflow: Could not insertKey\n";
				return;
			}

			// Inserting the new key at the end
			int i = Size;
			Array[Size++] = k;

			while (i != 0 && Array[Parent(i)] > Array[i]) {
				Swap(Array[i], Array[Parent(i)]);
				i = Parent(i);
			}
		}
		bool IsEmpty() { return Size <= 0; }
		T* Front() { assert(!IsEmpty()); return Array[0]; }
		void Clear() { Size = 0; }
	private:
		std::function<bool(const T&, const T&)> LessThanFunc;
		std::unique_ptr<T*[]> Array;
		size_t Capacity;
		size_t Size;
	};
}