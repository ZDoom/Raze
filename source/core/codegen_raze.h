#pragma once
#include "codegen.h"
#include "coreactor.h"

//==========================================================================
//
//	
//
//==========================================================================

class FxSetActionCall : public FxExpression
{
	FxExpression *Self;
	FxExpression *Arg;

public:

	FxSetActionCall(FxExpression *self, FxExpression* arg, const FScriptPosition &pos);
	~FxSetActionCall();
	FxExpression *Resolve(FCompileContext&);
	ExpEmit Emit(VMFunctionBuilder *build);
};

//==========================================================================
//
//	
//
//==========================================================================

class FxSetAICall : public FxExpression
{
	FxExpression *Self;
	FxExpression *Arg;

public:

	FxSetAICall(FxExpression *self, FxExpression* arg, const FScriptPosition &pos);
	~FxSetAICall();
	FxExpression *Resolve(FCompileContext&);
	ExpEmit Emit(VMFunctionBuilder *build);
};

//==========================================================================
//
//	
//
//==========================================================================

class FxSetMoveCall : public FxExpression
{
	FxExpression *Self;
	FxExpression *Arg;

public:

	FxSetMoveCall(FxExpression *self, FxExpression* arg, const FScriptPosition &pos);
	~FxSetMoveCall();
	FxExpression *Resolve(FCompileContext&);
	ExpEmit Emit(VMFunctionBuilder *build);
};

//==========================================================================
//
//	FxClassDefaults
//
//==========================================================================

class FxClassDefaults : public FxExpression
{
	FxExpression *obj;

public:
	FxClassDefaults(FxExpression *, const FScriptPosition &);
	~FxClassDefaults();
	FxExpression *Resolve(FCompileContext&);
	ExpEmit Emit(VMFunctionBuilder *build);
};

//==========================================================================
//
//	FxGetDefaultByType
//
//==========================================================================

class FxGetDefaultByType : public FxExpression
{
	FxExpression *Self;

public:

	FxGetDefaultByType(FxExpression *self);
	~FxGetDefaultByType();
	FxExpression *Resolve(FCompileContext&);
	ExpEmit Emit(VMFunctionBuilder *build);
};

//==========================================================================
//
//
//
//==========================================================================

class FxIsGameType : public FxExpression
{
	int state;

public:

	FxIsGameType(int arg, const FScriptPosition& pos);
	~FxIsGameType();
	FxExpression *Resolve(FCompileContext&);
	ExpEmit Emit(VMFunctionBuilder *build);
};

