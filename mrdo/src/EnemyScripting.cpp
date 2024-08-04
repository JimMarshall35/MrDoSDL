#include "EnemyScripting.h"
#include "Forth.h"
#include "CommonTypedefs.h"
#include <iostream>
#include <fstream>
#include "ForthStringHelpers.h"
#include "EnemyManager.h"
#include <cassert>
#include "TiledWorld.h"
#include "Character.h"

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
		return 0;
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

	Bool EnemyManager_ForthExposedMethodImplementations::FollowPath(ForthVm* vm)
	{
		// ( enemy pathFinishedCallback -- bHasReachedNewCell )
		ExecutionToken onPathFinished = (ExecutionToken)Pop();
		Enemy* enemy = (Enemy*)Pop();

		Cell* instructionPtrCached = vm->instructionPointer;
		bool enteredNewCell = Instance->FollowPathBase(*enemy, Instance->DeltaTime, [onPathFinished](Enemy& enemy) {
			Push((Cell)&enemy);
			DoExecutionToken(onPathFinished);
		});
		vm->instructionPointer = instructionPtrCached;

		Push(enteredNewCell ? -1 : 0);
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::GetEnemyTimerPtr(ForthVm* vm)
	{
		//( enemy -- &enemy->Timer )
		Enemy* enemy = (Enemy*)Pop();
		Push((Cell)&enemy->Timer);
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::GetDeltaT(ForthVm* vm)
	{
		// ( -- deltaT )
		Push((Cell)Instance->DeltaTime);
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::GetPushing(ForthVm* vm)
	{
		// ( enemy -- enemy->bIsPushing )
		Enemy* enemy = (Enemy*)Pop();
		Push(enemy->bPushing ? 1 : 0);
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::GetCurrentDirection(ForthVm* vm)
	{
		// ( enemy -- enemy->CurrentDirection )
		Enemy* enemy = (Enemy*)Pop();
		Push((Cell)enemy->CurrentDirection);
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::SetNormalAnimation(ForthVm* vm)
	{
		// ( enemy isPushing movementDirection -- )
		MovementDirection md = (MovementDirection)Pop();
		Cell isPushing = Pop();
		Enemy* enemy = (Enemy*)Pop();
		enemy->EnemyAnimator.CurrentAnimation = &EnemyManager::NormalEnemyAnimationTable[isPushing][(u32)md];
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::UpdateAnimator(ForthVm* vm)
	{
		// ( deltaT enemy -- )
		Enemy* enemy = (Enemy*)Pop();
		Cell deltaT = (Cell)Pop();
		enemy->EnemyAnimator.Update((float)deltaT / 1000.f);
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::GetEnemyTypePtr(ForthVm* vm)
	{
		// ( enemy -- &enemy->type )
		Enemy* enemy = (Enemy*)Pop();
		Push((Cell)&enemy->Type);
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::SetAnimationFrame(ForthVm* vm)
	{
		// ( enemy frame -- )
		Enemy* enemy = (Enemy*)Pop();
		Cell frame = (Cell)Pop();
		enemy->EnemyAnimator.OnAnimFrame = frame;
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::SetMorphingAnimation(ForthVm* vm)
	{
		// ( enemy movementDirection -- )
		MovementDirection md = (MovementDirection)Pop();
		Enemy* enemy = (Enemy*)Pop();
		enemy->EnemyAnimator.CurrentAnimation = &EnemyManager::TransformingToDiggerAnimationTable[(u32)md];
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::SetNewEnemyPathTo(ForthVm* vm)
	{
		// ( enemy destY destX -- pathFound )
		Cell destX = (Cell)Pop();
		Cell destY = (Cell)Pop();

		Enemy* enemy = (Enemy*)Pop();

		assert(destX >= 0);
		assert(destX < Instance->CachedTiledWorld->GetActiveLevelWidth());
		assert(destY >= 0);
		assert(destY < Instance->CachedTiledWorld->GetActiveLevelHeight());

		Push(Instance->SetNewPath(*enemy, {destX,destY}) ? True : False);
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::GetCharacterTile(ForthVm* vm)
	{
		// ( -- characterY characterX )
		Character* character = Instance->CachedCharacter;
		const ivec2& characterTile = character->GetTile();
		Push((Cell)characterTile.y);
		Push((Cell)characterTile.x);
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::SetNewDiggerPathTo(ForthVm* vm)
	{
		// ( enemy destinationY destinationX obstructionY obstructionX -- pathFound )
		Cell obsX = Pop();
		Cell obsY = Pop();
		Cell destX = Pop();
		Cell destY = Pop();
		Enemy* enemy = (Enemy*)Pop();
		Push(Instance->SetNewPathForDigger(*enemy, {destX, destY}, {obsX, obsY}) ? True : False);
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::GetCurrentDestination(ForthVm* vm)
	{
		// ( enemy -- destinationY destinationX )
		Enemy* enemy = (Enemy*)Pop();
		const ivec2& dest = enemy->PathBuffer[enemy->PathBufferDestinationIndex];
		Push((Cell)dest.y);
		Push((Cell)dest.x);
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::SetDiggerAnimation(ForthVm* vm)
	{
		// ( enemy movementDirection -- )
		MovementDirection md = (MovementDirection)Pop();
		Enemy* enemy = (Enemy*)Pop();
		enemy->EnemyAnimator.CurrentAnimation = &EnemyManager::DiggerAnimationTable[(u32)md];
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::SetExtraManAnimation(ForthVm* vm)
	{
		// ( enemy letter -- )
		char c = (char)Pop();
		Enemy* enemy = (Enemy*)Pop();
		enemy->EnemyAnimator.CurrentAnimation = &EnemyManager::ExtraMenAnimationTable[(u32)c];
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::ConnectAdjacentCells(ForthVm* vm)
	{
		// ( y1 x1 y2 x2 -- )
		Cell x2 = Pop();
		Cell y2 = Pop();
		Cell x1 = Pop();
		Cell y1 = Pop();
		Instance->CachedTiledWorld->ConnectAdjacentCells({x1,y1}, {x2, y2});
		return Bool();
	}

	Bool EnemyManager_ForthExposedMethodImplementations::GetCurrentCell(ForthVm* vm)
	{
		// ( -- y x )
		Enemy* enemy = (Enemy*)Pop();
		Push((Cell)enemy->CurrentCell.y);
		Push((Cell)enemy->CurrentCell.x);
		return Bool();
	}

	void EnemyManager_ForthExposedMethodImplementations::RegisterForthFunctions()
	{
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::FollowPath, "FollowPath");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::GetEnemyTimerPtr, "GetTimerPtr");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::GetDeltaT, "GetDeltaT");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::GetPushing, "GetPushing");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::GetCurrentDirection, "GetCurrentDirection");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::SetNormalAnimation, "SetNormalAnimation");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::UpdateAnimator, "UpdateAnimator");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::GetEnemyTypePtr, "GetEnemyTypePtr");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::SetAnimationFrame, "SetAnimationFrame");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::SetMorphingAnimation, "SetMorphingAnimation");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::SetNewEnemyPathTo, "SetPathTo");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::GetCharacterTile, "GetCharacterTile");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::SetNewDiggerPathTo, "SetNewDiggerPath");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::GetCurrentDestination, "GetCurrentDestination");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::SetDiggerAnimation, "SetDiggerAnimation");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::ConnectAdjacentCells, "ConnectAdjacentCells");
		RegisterCFunction(&EnemyManager_ForthExposedMethodImplementations::GetCurrentCell, "GetCurrentCell");
	}
}


