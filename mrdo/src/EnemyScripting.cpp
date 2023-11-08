#include "EnemyScripting.h"
#include "Forth.h"
#include "CommonTypedefs.h"
#include <iostream>
#include <fstream>
#include "ForthStringHelpers.h"
#include "EnemyManager.h"

namespace EnemyScripting
{
	static ForthVm VM;
	
	static Cell* VMMemory = nullptr;

	static Cell* VMIntStack = nullptr;
	static Cell* VMReturnStack = nullptr;
	static Cell* VMDictionary = nullptr;
	static size_t VMReturnStackSizeCells = 0;
	static size_t VMIntStackSizeCells = 0;
	static size_t VMDictionarySizeCells = 0;

	EnemyManager* EnemyManager_ForthExposedMethodImplementations::Instance = nullptr;

	static int PutChar(int val)
	{
		std::cout << (char)val;
		return 1;
	}
	
	static int GetChar()
	{
		return std::cin.get();
	}

	void InitScripting(size_t scriptVmDictionaryMemory, size_t scriptVmIntStackSize, size_t scriptVmReturnStackSize)
	{
		VMMemory = new Cell[scriptVmDictionaryMemory + scriptVmIntStackSize + scriptVmReturnStackSize];
		VMIntStack = VMMemory;
		VMReturnStack = VMMemory + scriptVmIntStackSize;
		VMDictionary = VMMemory + scriptVmIntStackSize + scriptVmReturnStackSize;
		VM = Forth_Initialise(VMMemory + scriptVmIntStackSize + scriptVmReturnStackSize,
			scriptVmDictionaryMemory,
			VMIntStack,
			scriptVmIntStackSize,
			VMReturnStack,
			scriptVmReturnStackSize,
			&PutChar,
			&GetChar);

	}

	void DoFile(const std::string& filePath)
	{
		ForthVMLoadSourceFile(&VM, filePath.c_str());
	}

	void RegisterCFunction(ForthCFunc func, const char* name)
	{
		Forth_RegisterCFunc(&VM, func, name, False);
	}

	ExecutionToken FindExecutionToken(const char* name)
	{
		return Forth_SearchExecutionToken(&VM, name);
	}

	void DoExecutionToken(ExecutionToken token)
	{
		Forth_DoExecutionToken(&VM, token);
	}

	void ForthDoString(const std::string& inputString)
	{
		Forth_DoString(&VM, inputString.c_str());
	}

	void DeInitScripting()
	{
		delete[] VMMemory;
	}

	
	

	Cell Pop()
	{
		if (VM.intStackTop - 1 >= VM.intStack)
		{
			return *(--VM.intStackTop);
		}
		else
		{
			std::cout << "[FORTH] Int stack empty!\n";
		}
	}

	void Push(Cell cell)
	{
		if (VM.intStackTop + 1 <= VMReturnStack)
		{
			*(VM.intStackTop++) = cell;
		}
		else
		{
			std::cout << "[FORTH] Int stack full!\n";
		}
	}

	template<typename T>
	T Pop()
	{
		return static_cast<T>(Pop());
	}

	template<typename T>
	void Push(T t)
	{
		Push(static_assert<Cell>(t));
	}

	
	Bool EnemyManager_ForthExposedMethodImplementations::FollowPath(ForthVm* vm)
	{
		Enemy* enemy = Pop<Enemy*>();
		ExecutionToken onPathFinished = Pop<ExecutionToken>();
		Instance->FollowPathBase(*enemy, Instance->DeltaTime, [onPathFinished](Enemy& enemy) {
			Push<Enemy*>(&enemy);
			DoExecutionToken(onPathFinished);
		});
		return Bool();
	}

}


