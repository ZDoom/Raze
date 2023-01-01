#pragma once
#include "codegen.h"
#include "coreactor.h"


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

