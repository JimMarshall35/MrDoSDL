
#ifndef FORTH2
#define FORTH2
#ifdef __cplusplus
extern "C" {
#endif


#include "ForthCommonTypedefs.h"


	// only obtain a forth Vm like this
	ForthVm Forth_Initialise(
		Cell* memoryForCompiledWordsAndVariables,
		UCell memorySize,
		Cell* intStack,
		UCell intStackSize,
		Cell* returnStack,
		UCell returnStackSize,
		ForthPutChar putc,
		ForthGetChar getc);

	Bool Forth_DoString(ForthVm* vm, const char* inputString);
	void Forth_RegisterCFunc(ForthVm* vm, ForthCFunc function, const char* name, Bool isImmediate);

	// mr do additions
	void Forth_DoExecutionToken(ForthVm* vm, ExecutionToken token);
	ExecutionToken Forth_SearchExecutionToken(ForthVm* vm, const char* tokenName);
	// mr do additions end
#ifdef __cplusplus
} // closing brace for extern "C"
#endif
#endif