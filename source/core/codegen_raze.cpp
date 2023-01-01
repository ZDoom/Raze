/*
** codegen.cpp
**
** Compiler backend / code generation for ZScript
**
**---------------------------------------------------------------------------
** Copyright 2008-2022 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <stdlib.h>
#include "cmdlib.h"
#include "codegen.h"
#include "codegen_raze.h"
#include "v_text.h"
#include "filesystem.h"
#include "v_video.h"
#include "utf8.h"
#include "texturemanager.h"
#include "m_random.h"
#include "v_font.h"
#include "gamecontrol.h"
#include "gi.h"
#include "games/duke/src/duke3d.h"

PFunction* FindBuiltinFunction(FName funcname);

//==========================================================================
//
//
//
//==========================================================================

bool isActor(PContainerType* type)
{
	auto cls = PType::toClass(type);
	return cls ? cls->Descriptor->IsDescendantOf(RUNTIME_CLASS(DCoreActor)) : false;
}

//==========================================================================
//
//
//
//==========================================================================

static FxExpression* CheckForDefault(FxIdentifier* func, FCompileContext& ctx)
{
	auto& ScriptPosition = func->ScriptPosition;

	if (func->Identifier == NAME_Default)
	{
		if (ctx.Function == nullptr)
		{
			ScriptPosition.Message(MSG_ERROR, "Unable to access class defaults from constant declaration");
			delete func;
			return nullptr;
		}
		if (ctx.Function->Variants[0].SelfClass == nullptr)
		{
			ScriptPosition.Message(MSG_ERROR, "Unable to access class defaults from static function");
			delete func;
			return nullptr;
		}
		if (!isActor(ctx.Function->Variants[0].SelfClass))
		{
			ScriptPosition.Message(MSG_ERROR, "'Default' requires an actor type.");
			delete func;
			return nullptr;
		}

		FxExpression* x = new FxClassDefaults(new FxSelf(ScriptPosition), ScriptPosition);
		delete func;
		return x->Resolve(ctx);
	}
	return func;
}

//==========================================================================
//
//
//
//==========================================================================

static FxExpression *ResolveForDefault(FxIdentifier *expr, FxExpression*& object, PContainerType* objtype, FCompileContext &ctx)
{

	if (expr->Identifier == NAME_Default)
	{
		if (!isActor(objtype))
		{
			expr->ScriptPosition.Message(MSG_ERROR, "'Default' requires an actor type.");
			delete object;
			object = nullptr;
			return nullptr;
		}

		FxExpression * x = new FxClassDefaults(object, expr->ScriptPosition);
		object = nullptr;
		delete expr;
		return x->Resolve(ctx);
	}
	return expr;
}


//==========================================================================
//
//
//
//==========================================================================

FxExpression* CheckForMemberDefault(FxStructMember *func, FCompileContext &ctx)
{
	auto& membervar = func->membervar;
	auto& classx = func->classx;
	auto& ScriptPosition = func->ScriptPosition;

	if (membervar->SymbolName == NAME_Default)
	{
		if (!classx->ValueType->isObjectPointer()
			|| !static_cast<PObjectPointer *>(classx->ValueType)->PointedClass()->IsDescendantOf(RUNTIME_CLASS(DCoreActor)))
		{
			ScriptPosition.Message(MSG_ERROR, "'Default' requires an actor type");
			delete func;
			return nullptr;
		}
		FxExpression * x = new FxClassDefaults(classx, ScriptPosition);
		classx = nullptr;
		delete func;
		return x->Resolve(ctx);
	}
	return func;
}

//==========================================================================
//
//
//
//==========================================================================

bool CheckArgSize(FName fname, FArgumentList &args, int min, int max, FScriptPosition &sc);

static FxExpression *ResolveGlobalCustomFunction(FxFunctionCall *func, FCompileContext &ctx)
{
	auto& ScriptPosition = func->ScriptPosition;
	if (func->MethodName == NAME_GetDefaultByType)
	{
		if (CheckArgSize(NAME_GetDefaultByType, func->ArgList, 1, 1, ScriptPosition))
		{
			auto newfunc = new FxGetDefaultByType(func->ArgList[0]);
			func->ArgList[0] = nullptr;
			delete func;
			return newfunc->Resolve(ctx);
		}
	}
	// the following 3 are just for compile time argument validation, they do not affect code generation.
	else if (func->MethodName == NAME_SetAction && isDukeEngine())
	{
		auto cls = ctx.Function->Variants[0].SelfClass;
		if (cls == nullptr || !cls->isClass()) return func;
		auto clas = static_cast<PClassType*>(cls)->Descriptor;
		if (!clas->IsDescendantOf(RUNTIME_CLASS(Duke3d::DDukeActor))) return func;
		if (CheckArgSize(NAME_SetAction, func->ArgList, 1, 1, ScriptPosition))
		{
			FxExpression* x;
			x = func->ArgList[0] = func->ArgList[0]->Resolve(ctx);
			if (x && x->isConstant() && (x->ValueType == TypeName || x->ValueType == TypeString))
			{
				auto c = static_cast<FxConstant*>(x);
				auto nm = c->GetValue().GetName();
				if (nm == NAME_None) return func;
				int pos = Duke3d::LookupAction(clas, nm);
				if (pos == 0)
				{
					ScriptPosition.Message(MSG_ERROR, "Invalid action '%s'", nm.GetChars());
					delete func;
					return nullptr;
				}
			}
		}
	}
	else if (func->MethodName == NAME_SetAI && isDukeEngine())
	{
		auto cls = ctx.Function->Variants[0].SelfClass;
		if (cls == nullptr || !cls->isClass()) return func;
		auto clas = static_cast<PClassType*>(cls)->Descriptor;
		if (!clas->IsDescendantOf(RUNTIME_CLASS(Duke3d::DDukeActor))) return func;
		if (CheckArgSize(NAME_SetAI, func->ArgList, 1, 1, ScriptPosition))
		{
			FxExpression* x;
			x = func->ArgList[0] = func->ArgList[0]->Resolve(ctx);
			if (x && x->isConstant() && (x->ValueType == TypeName || x->ValueType == TypeString))
			{
				auto c = static_cast<FxConstant*>(x);
				auto nm = c->GetValue().GetName();
				if (nm == NAME_None) return func;
				int pos = Duke3d::LookupAI(clas, nm);
				if (pos == 0)
				{
					ScriptPosition.Message(MSG_ERROR, "Invalid AI '%s'", nm.GetChars());
					delete func;
					return nullptr;
				}
			}
		}
	}
	else if (func->MethodName == NAME_SetMove && isDukeEngine())
	{
		auto cls = ctx.Function->Variants[0].SelfClass;
		if (cls == nullptr || !cls->isClass()) return func;
		auto clas = static_cast<PClassType*>(cls)->Descriptor;
		if (!clas->IsDescendantOf(RUNTIME_CLASS(Duke3d::DDukeActor))) return func;
		if (CheckArgSize(NAME_SetMove, func->ArgList, 1, 2, ScriptPosition))
		{
			FxExpression* x;
			x = func->ArgList[0] = func->ArgList[0]->Resolve(ctx);
			if (x && x->isConstant() && (x->ValueType == TypeName || x->ValueType == TypeString))
			{
				auto c = static_cast<FxConstant*>(x);
				auto nm = c->GetValue().GetName();
				if (nm == NAME_None) return func;
				int pos = Duke3d::LookupMove(clas, nm);
				if (pos == 0)
				{
					ScriptPosition.Message(MSG_ERROR, "Invalid move '%s'", nm.GetChars());
					delete func;
					return nullptr;
				}
			}
		}
	}
	// most of these can be resolved right here. Only isWorldTour, isPlutoPak and isShareware can not if Duke is being played.
	else if (func->MethodName == NAME_isDuke)
	{
		return new FxConstant(isDuke(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isNam)
	{
		return new FxConstant(isNam(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isNamWW2GI)
	{
		return new FxConstant(isNamWW2GI(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isWW2GI)
	{
		return new FxConstant(isWW2GI(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isRR)
	{
		return new FxConstant(isRR(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isRRRA)
	{
		return new FxConstant(isRRRA(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isRoute66)
	{
		return new FxConstant(isRoute66(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isWorldTour)
	{
		if (!isDuke()) return new FxConstant(false, ScriptPosition);
		else return new FxIsGameType(GAMEFLAG_WORLDTOUR, ScriptPosition);
	}
	else if (func->MethodName == NAME_isPlutoPak)
	{
		if (!isDuke()) return new FxConstant(false, ScriptPosition);
		else return new FxIsGameType(GAMEFLAG_PLUTOPAK, ScriptPosition);
	}
	else if (func->MethodName == NAME_isShareware)
	{
		if (!isDuke()) return new FxConstant(isShareware(), ScriptPosition);
		else return new FxIsGameType(GAMEFLAG_SHAREWARE, ScriptPosition);
	}
	else if (func->MethodName == NAME_isVacation)
	{
		return new FxConstant(isVacation(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isDukeLike)
	{
		return new FxConstant(isDukeLike(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isDukeEngine)
	{
		return new FxConstant(isDukeEngine(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isBlood)
	{
		return new FxConstant(isBlood(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isSW)
	{
		return new FxConstant(isSWALL(), ScriptPosition);
	}
	else if (func->MethodName == NAME_isExhumed)
	{
		return new FxConstant(isExhumed(), ScriptPosition);
	}

	return func;
}

//==========================================================================
//
// code generation is only performed for some Duke checks that depend on CON
// which cannot be resolved at compile time.
//
//==========================================================================

FxIsGameType::FxIsGameType(int arg, const FScriptPosition &pos)
: FxExpression(EFX_ActionSpecialCall, pos)
{
	state = arg;
}

//==========================================================================
//
//
//
//==========================================================================

FxIsGameType::~FxIsGameType()
{
}

//==========================================================================
//
//
//
//==========================================================================

FxExpression *FxIsGameType::Resolve(FCompileContext& ctx)
{
	CHECKRESOLVED();
	ValueType = TypeBool;
	return this;
}


//==========================================================================
//
//
//
//==========================================================================

ExpEmit FxIsGameType::Emit(VMFunctionBuilder *build)
{
	ExpEmit obj(build, REGT_POINTER);

	auto addr = (intptr_t)&gameinfo.gametype;
	assert(state != 0);
	while (state >= 256)
	{
		state >>= 8;
		addr++;
	}
	build->Emit(OP_LKP, obj.RegNum, build->GetConstantAddress((void*)addr));
	ExpEmit loc(build, REGT_INT);
	build->Emit(OP_LBIT, loc.RegNum, obj.RegNum, state);
	obj.Free(build);
	return loc;
}

//==========================================================================
//
//
//
//==========================================================================

FxClassDefaults::FxClassDefaults(FxExpression *X, const FScriptPosition &pos)
	: FxExpression(EFX_ClassDefaults, pos)
{
	obj = X;
}

FxClassDefaults::~FxClassDefaults()
{
	SAFE_DELETE(obj);
}


//==========================================================================
//
//
//
//==========================================================================

FxExpression *FxClassDefaults::Resolve(FCompileContext& ctx)
{
	CHECKRESOLVED();
	SAFE_RESOLVE(obj, ctx);
	assert(obj->ValueType->isRealPointer());
	ValueType = NewPointer(obj->ValueType->toPointer()->PointedType, true);
	return this;
}

//==========================================================================
//
//
//
//==========================================================================

ExpEmit FxClassDefaults::Emit(VMFunctionBuilder *build)
{
	ExpEmit ob = obj->Emit(build);
	ob.Free(build);
	ExpEmit meta(build, REGT_POINTER);
	build->Emit(OP_CLSS, meta.RegNum, ob.RegNum);
	build->Emit(OP_LP, meta.RegNum, meta.RegNum, build->GetConstantInt(myoffsetof(PClass, Defaults)));
	return meta;

}

//==========================================================================
//
//
//==========================================================================

FxGetDefaultByType::FxGetDefaultByType(FxExpression *self)
	:FxExpression(EFX_GetDefaultByType, self->ScriptPosition)
{
	Self = self;
}

FxGetDefaultByType::~FxGetDefaultByType()
{
	SAFE_DELETE(Self);
}

FxExpression *FxGetDefaultByType::Resolve(FCompileContext &ctx)
{
	SAFE_RESOLVE(Self, ctx);
	PClass *cls = nullptr;

	if (Self->ValueType == TypeString || Self->ValueType == TypeName)
	{
		if (Self->isConstant())
		{
			cls = PClass::FindActor(static_cast<FxConstant *>(Self)->GetValue().GetName());
			if (cls == nullptr)
			{
				ScriptPosition.Message(MSG_ERROR, "GetDefaultByType() requires an actor class type, but got %s", static_cast<FxConstant *>(Self)->GetValue().GetString().GetChars());
				delete this;
				return nullptr;
			}
			Self = new FxConstant(cls, NewClassPointer(cls), ScriptPosition);
		}
		else
		{
			// this is the ugly case. We do not know what we have and cannot do proper type casting.
			// For now error out and let this case require explicit handling on the user side.
			ScriptPosition.Message(MSG_ERROR, "GetDefaultByType() requires an actor class type, but got %s", static_cast<FxConstant *>(Self)->GetValue().GetString().GetChars());
			delete this;
			return nullptr;
		}
	}
	else
	{
		auto cp = PType::toClassPointer(Self->ValueType);
		if (cp == nullptr || !cp->ClassRestriction->IsDescendantOf(RUNTIME_CLASS(DCoreActor)))
		{
			ScriptPosition.Message(MSG_ERROR, "GetDefaultByType() requires an actor class type");
			delete this;
			return nullptr;
		}
		cls = cp->ClassRestriction;
	}
	ValueType = NewPointer(cls, true);
	return this;
}

ExpEmit FxGetDefaultByType::Emit(VMFunctionBuilder *build)
{
	ExpEmit op = Self->Emit(build);
	op.Free(build);
	ExpEmit to(build, REGT_POINTER);
	if (op.Konst)
	{
		build->Emit(OP_LKP, to.RegNum, op.RegNum);
		op = to;
	}
	build->Emit(OP_LP, to.RegNum, op.RegNum, build->GetConstantInt(myoffsetof(PClass, Defaults)));
	return to;
}




void SetRazeCompileEnvironment()
{
	compileEnvironment.CheckSpecialIdentifier = CheckForDefault;
	compileEnvironment.ResolveSpecialIdentifier = ResolveForDefault;
	compileEnvironment.CheckSpecialMember = CheckForMemberDefault;
	compileEnvironment.CheckCustomGlobalFunctions = ResolveGlobalCustomFunction;
}
