#pragma once
#include <string>
#include <type_traits>
#include "ForthCommonTypedefs.h"
class EnemyManager;
namespace EnemyScripting
{
	void InitScripting(size_t scriptVmDictionaryMemory, size_t scriptVmIntStackSize, size_t scriptVmReturnStackSize);
	void DoFile(const std::string& filePath);
	void DeInitScripting();
	void ForthDoString(const std::string& inputString);
	void RegisterCFunction(ForthCFunc func, const char* name);
	ExecutionToken FindExecutionToken(const char* name);
	void DoExecutionToken(ExecutionToken token);
	Cell Pop();
	void Push(Cell cell);

	class EnemyManager_ForthExposedMethodImplementations
	{
	public:
		static EnemyManager* Instance;
		static Bool FollowPath(ForthVm* vm);
		static Bool GetEnemyTimerPtr(ForthVm* vm);
		static Bool GetDeltaT(ForthVm* vm);
		static Bool GetPushing(ForthVm* vm);
		static Bool GetCurrentDirection(ForthVm* vm);
		static Bool SetNormalAnimation(ForthVm* vm);
		static Bool UpdateAnimator(ForthVm* vm);
		static Bool GetEnemyTypePtr(ForthVm* vm);
		static Bool SetAnimationFrame(ForthVm* vm);
		static Bool SetMorphingAnimation(ForthVm* vm);
		static Bool SetNewEnemyPathTo(ForthVm* vm);
		static Bool GetCharacterTile(ForthVm* vm);
		static Bool SetNewDiggerPathTo(ForthVm* vm);
		static Bool GetCurrentDestination(ForthVm* vm);
		static Bool SetDiggerAnimation(ForthVm* vm);
		static Bool SetExtraManAnimation(ForthVm* vm);
		static Bool ConnectAdjacentCells(ForthVm* vm);
		static Bool GetCurrentCell(ForthVm* vm);

		static void RegisterForthFunctions();
	};
}