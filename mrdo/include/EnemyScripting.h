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
	class EnemyManager_ForthExposedMethodImplementations
	{
	public:
		static EnemyManager* Instance;
		static Bool FollowPath(ForthVm* vm);

	};
}