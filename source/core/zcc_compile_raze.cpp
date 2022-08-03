/*
** zcc_compile_raze.cpp
**
** contains the Raze specific parts of the script parser, i.e.
** actor property definitions and associated content.
**
**---------------------------------------------------------------------------
** Copyright 2016-2022 Christoph Oelckers
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

#include "coreactor.h"
#include "c_console.h"
#include "filesystem.h"
#include "zcc_parser.h"
#include "zcc-parse.h"
#include "zcc_compile_raze.h"
#include "v_text.h"
#include "v_video.h"
#include "actorinfo.h"
#include "thingdef.h"


bool isActor(PContainerType *type);
void AddActorInfo(PClass *cls);
int GetIntConst(FxExpression* ex, FCompileContext& ctx);
double GetFloatConst(FxExpression* ex, FCompileContext& ctx);

//==========================================================================
//
// ZCCCompiler :: Compile
//
// Compile everything defined at this level.
//
//==========================================================================

int ZCCRazeCompiler::Compile()
{
	CreateClassTypes();
	CreateStructTypes();
	CompileAllConstants();
	CompileAllFields();
	CompileAllProperties();
	InitDefaults();
	InitFunctions();
	return FScriptPosition::ErrorCounter;
}


//==========================================================================
//
// ZCCCompiler :: CompileAllProperties
//
// builds the property lists of all actor classes
//
//==========================================================================

void ZCCRazeCompiler::CompileAllProperties()
{
	for (auto c : Classes)
	{
		if (c->Properties.Size() > 0)
			CompileProperties(c->ClassType(), c->Properties, c->Type()->TypeName);

		if (c->FlagDefs.Size() > 0)
			CompileFlagDefs(c->ClassType(), c->FlagDefs, c->Type()->TypeName);

	}
}

//==========================================================================
//
// ZCCCompiler :: CompileProperties
//
// builds the internal structure of a single class or struct
//
//==========================================================================

bool ZCCRazeCompiler::CompileProperties(PClass *type, TArray<ZCC_Property *> &Properties, FName prefix)
{
	if (!type->IsDescendantOf(RUNTIME_CLASS(DCoreActor)))
	{
		Error(Properties[0], "Properties can only be defined for actors");
		return false;
	}
	for(auto p : Properties)
	{
		TArray<PField *> fields;
		ZCC_Identifier *id = (ZCC_Identifier *)p->Body;

		if (FName(p->NodeName) == FName("prefix") && fileSystem.GetFileContainer(Lump) == 0)
		{
			// only for internal definitions: Allow setting a prefix. This is only for compatiblity with the old DECORATE property parser, but not for general use.
			prefix = id->Id;
		}
		else
		{
			do
			{
				auto f = dyn_cast<PField>(type->FindSymbol(id->Id, true));
				if (f == nullptr)
				{
					Error(id, "Variable %s not found in %s", FName(id->Id).GetChars(), type->TypeName.GetChars());
				}
				fields.Push(f);
				id = (ZCC_Identifier*)id->SiblingNext;
			} while (id != p->Body);

			FString qualifiedname;
			// Store the full qualified name and prepend some 'garbage' to the name so that no conflicts with other symbol types can happen.
			// All these will be removed from the symbol table after the compiler finishes to free up the allocated space.
			FName name = FName(p->NodeName);
			if (prefix == NAME_None) qualifiedname.Format("@property@%s", name.GetChars());
			else qualifiedname.Format("@property@%s.%s", prefix.GetChars(), name.GetChars());

			fields.ShrinkToFit();
			if (!type->VMType->Symbols.AddSymbol(Create<PProperty>(qualifiedname, fields)))
			{
				Error(id, "Unable to add property %s to class %s", FName(p->NodeName).GetChars(), type->TypeName.GetChars());
			}
		}
	}
	return true;
}

//==========================================================================
//
// ZCCCompiler :: CompileProperties
//
// builds the internal structure of a single class or struct
//
//==========================================================================

bool ZCCRazeCompiler::CompileFlagDefs(PClass *type, TArray<ZCC_FlagDef *> &Properties, FName prefix)
{
	if (!type->IsDescendantOf(RUNTIME_CLASS(DCoreActor)))
	{
		Error(Properties[0], "Flags can only be defined for actors");
		return false;
	}
	for (auto p : Properties)
	{
		PField *field;
		FName referenced = FName(p->RefName);

		if (FName(p->NodeName) == FName("prefix") && fileSystem.GetFileContainer(Lump) == 0)
		{
			// only for internal definitions: Allow setting a prefix. This is only for compatiblity with the old DECORATE property parser, but not for general use.
			prefix = referenced;
		}
		else
		{
			if (referenced != NAME_None)
			{
				field = dyn_cast<PField>(type->FindSymbol(referenced, true));
				if (field == nullptr)
				{
					Error(p, "Variable %s not found in %s", referenced.GetChars(), type->TypeName.GetChars());
				}
				else if (!field->Type->isInt() || field->Type->Size != 4)
				{
					Error(p, "Variable %s in %s must have a size of 4 bytes for use as flag storage", referenced.GetChars(), type->TypeName.GetChars());
				}
			}
			else field = nullptr;


			FString qualifiedname;
			// Store the full qualified name and prepend some 'garbage' to the name so that no conflicts with other symbol types can happen.
			// All these will be removed from the symbol table after the compiler finishes to free up the allocated space.
			FName name = FName(p->NodeName);
			for (int i = 0; i < 2; i++)
			{
				if (i == 0) qualifiedname.Format("@flagdef@%s", name.GetChars());
				else
				{
					if (prefix == NAME_None) continue;
					qualifiedname.Format("@flagdef@%s.%s", prefix.GetChars(), name.GetChars());
				}

				if (!type->VMType->Symbols.AddSymbol(Create<PPropFlag>(qualifiedname, field, p->BitValue, i == 0 && prefix != NAME_None)))
				{
					Error(p, "Unable to add flag definition %s to class %s", FName(p->NodeName).GetChars(), type->TypeName.GetChars());
				}
			}

			if (field != nullptr)
				type->VMType->AddNativeField(FStringf("b%s", name.GetChars()), TypeSInt32, field->Offset, 0, 1 << p->BitValue);
		} 
	}
	return true;
}

//==========================================================================
//
// Parses an actor property's parameters and calls the handler
//
//==========================================================================

void ZCCRazeCompiler::DispatchProperty(FPropertyInfo *prop, ZCC_PropertyStmt *property, DCoreActor *defaults, Baggage &bag)
{
	static TArray<FPropParam> params;
	static TArray<FString> strings;

	params.Clear();
	strings.Clear();
	params.Reserve(1);
	params[0].i = 0;
	if (prop->params[0] != '0')
	{
		if (property->Values == nullptr)
		{
			Error(property, "%s: arguments missing", prop->name);
			return;
		}
		const char * p = prop->params;
		auto exp = property->Values;

		FCompileContext ctx(OutNamespace, bag.Info->VMType, false, mVersion);
		while (true)
		{
			FPropParam conv;
			FPropParam pref;

			FxExpression *ex = ConvertNode(exp);
			ex = ex->Resolve(ctx);
			if (ex == nullptr)
			{
				return;
			}
			else if (!ex->isConstant())
			{
				// If we get TypeError, there has already been a message from deeper down so do not print another one.
				if (exp->Type != TypeError) Error(exp, "%s: non-constant parameter", prop->name);
				return;
			}
			conv.s = nullptr;
			pref.s = nullptr;
			pref.i = -1;
			switch ((*p) & 223)
			{

			case 'X':	// Expression in parentheses or number. We only support the constant here. The function will have to be handled by a separate property to get past the parser.
				conv.i = GetIntConst(ex, ctx);
				params.Push(conv);
				conv.exp = nullptr;
				break;

			case 'I':
				conv.i = GetIntConst(ex, ctx);
				break;

			case 'F':
				conv.d = GetFloatConst(ex, ctx);
				break;

			case 'Z':	// an optional string. Does not allow any numeric value.
				if (ex->ValueType != TypeString)
				{
					// apply this expression to the next argument on the list.
					params.Push(conv);
					params[0].i++;
					p++;
					continue;
				}
				conv.s = GetStringConst(ex, ctx);
				break;

			case 'C':	// this parser accepts colors only in string form.
				pref.i = 1;
				[[fallthrough]];
			case 'S':
			case 'T': // a filtered string (ZScript only parses filtered strings so there's nothing to do here.)
				conv.s = GetStringConst(ex, ctx);
				break;

			case 'L':	// Either a number or a list of strings
				if (ex->ValueType != TypeString)
				{
					pref.i = 0;
					conv.i = GetIntConst(ex, ctx);
				}
				else
				{
					pref.i = 1;
					params.Push(pref);
					params[0].i++;

					do
					{
						conv.s = GetStringConst(ex, ctx);
						if (conv.s != nullptr)
						{
							params.Push(conv);
							params[0].i++;
						}
						exp = static_cast<ZCC_Expression *>(exp->SiblingNext);
						if (exp != property->Values)
						{
							ex = ConvertNode(exp);
							ex = ex->Resolve(ctx);
							if (ex == nullptr) return;
						}
					} while (exp != property->Values);
					goto endofparm;
				}
				break;

			default:
				assert(false);
				break;

			}
			if (pref.i != -1)
			{
				params.Push(pref);
				params[0].i++;
			}
			params.Push(conv);
			params[0].i++;
			exp = static_cast<ZCC_Expression *>(exp->SiblingNext);
		endofparm:
			p++;
			// Skip the DECORATE 'no comma' marker
			if (*p == '_') p++;

			if (*p == 0)
			{
				if (exp != property->Values)
				{
					Error(property, "Too many values for '%s'", prop->name);
					return;
				}
				break;
			}
			else if (exp == property->Values)
			{
				if (*p < 'a')
				{
					Error(property, "Insufficient parameters for %s", prop->name);
					return;
				}
				break;
			}
		}
	}
	// call the handler
	try
	{
		prop->Handler(defaults, bag.Info, bag, &params[0]);
	}
	catch (CRecoverableError &error)
	{
		Error(property, "%s", error.GetMessage());
	}
}


//==========================================================================
//
// Parses an actor property's parameters and calls the handler
//
//==========================================================================

void ZCCRazeCompiler::DispatchScriptProperty(PProperty *prop, ZCC_PropertyStmt *property, DCoreActor *defaults, Baggage &bag)
{
	ZCC_ExprConstant one;
	unsigned parmcount = 1;
	ZCC_TreeNode *x = property->Values;
	if (x == nullptr)
	{
		parmcount = 0;
	}
	else
	{
		while (x->SiblingNext != property->Values)
		{
			x = x->SiblingNext;
			parmcount++;
		}
	}
	if (parmcount == 0 && prop->Variables.Size() == 1 && prop->Variables[0]->Type == TypeBool)
	{
		// allow boolean properties to have the parameter omitted
		memset(&one, 0, sizeof(one));
		one.SourceName = property->SourceName;	// This may not be null!
		one.Operation = PEX_ConstValue;
		one.NodeType = AST_ExprConstant;
		one.Type = TypeBool;
		one.IntVal = 1;
		property->Values = &one;
	}
	else if (parmcount != prop->Variables.Size())
	{
		Error(x == nullptr? property : x, "Argument count mismatch: Got %u, expected %u", parmcount, prop->Variables.Size());
		return;
	}

	auto exp = property->Values;
	FCompileContext ctx(OutNamespace, bag.Info->VMType, false, mVersion);
	for (auto f : prop->Variables)
	{
		void *addr;
		if (f == nullptr)
		{
			// This variable was missing. The error had been reported for the property itself already.
			return;
		}

		if (f->Flags & VARF_Meta)
		{
			addr = ((char*)bag.Info->Meta) + f->Offset;
		}
		else
		{
			addr = ((char*)defaults) + f->Offset;
		}

		FxExpression *ex = ConvertNode(exp);
		ex = ex->Resolve(ctx);
		if (ex == nullptr)
		{
			return;
		}
		else if (!ex->isConstant())
		{
			// If we get TypeError, there has already been a message from deeper down so do not print another one.
			if (exp->Type != TypeError) Error(exp, "%s: non-constant parameter", prop->SymbolName.GetChars());
			return;
		}

		if (f->Type == TypeBool)
		{
			static_cast<PBool*>(f->Type)->SetValue(addr, !!GetIntConst(ex, ctx));
		}
		else if (f->Type == TypeName)
		{
			*(FName*)addr = GetStringConst(ex, ctx);
		}
		else if (f->Type == TypeSound)
		{
			*(FSoundID*)addr = GetStringConst(ex, ctx);
		}
		else if (f->Type == TypeColor && ex->ValueType == TypeString)	// colors can also be specified as ints.
		{
			*(PalEntry*)addr = V_GetColor(GetStringConst(ex, ctx), &ex->ScriptPosition);
		}
		else if (f->Type->isIntCompatible())
		{
			static_cast<PInt*>(f->Type)->SetValue(addr, GetIntConst(ex, ctx));
		}
		else if (f->Type->isFloat())
		{
			static_cast<PFloat*>(f->Type)->SetValue(addr, GetFloatConst(ex, ctx));
		}
		else if (f->Type == TypeString)
		{
			*(FString*)addr = GetStringConst(ex, ctx);
		}
		else if (f->Type->isClassPointer())
		{
			auto clsname = GetStringConst(ex, ctx);
			if (*clsname == 0 || !stricmp(clsname, "none"))
			{
				*(PClass**)addr = nullptr;
			}
			else
			{
				auto cls = PClass::FindClass(clsname);
				auto cp = static_cast<PClassPointer*>(f->Type);
				if (cls == nullptr)
				{
					cls = cp->ClassRestriction->FindClassTentative(clsname);
				}
				else if (!cls->IsDescendantOf(cp->ClassRestriction))
				{
					Error(property, "class %s is not compatible with property type %s", clsname, cp->ClassRestriction->TypeName.GetChars());
				}
				*(PClass**)addr = cls;
			}
		}
		else
		{
			Error(property, "unhandled property type %s", f->Type->DescriptiveName());
		}
		exp->ToErrorNode();	// invalidate after processing.
		exp = static_cast<ZCC_Expression *>(exp->SiblingNext);
	}
}

//==========================================================================
//
// Parses an actor property
//
//==========================================================================

void ZCCRazeCompiler::ProcessDefaultProperty(PClassActor *cls, ZCC_PropertyStmt *prop, Baggage &bag)
{
	auto namenode = prop->Prop;
	FString propname;

	if (namenode->SiblingNext == namenode)
	{
		// a one-name property
		propname = FName(namenode->Id).GetChars();

	}
	else if (namenode->SiblingNext->SiblingNext == namenode)
	{
		// a two-name property
		propname << FName(namenode->Id).GetChars() << "." << FName(static_cast<ZCC_Identifier *>(namenode->SiblingNext)->Id).GetChars();
	}
	else
	{
		Error(prop, "Property name may at most contain two parts");
		return;
	}


	FPropertyInfo *property = FindProperty(propname);

	if (property != nullptr && property->category != CAT_INFO)
	{
		auto pcls = PClass::FindActor(property->clsname);
		if (cls->IsDescendantOf(pcls))
		{
			DispatchProperty(property, prop, (DCoreActor *)bag.Info->Defaults, bag);
		}
		else
		{
			Error(prop, "'%s' requires an actor of type '%s'\n", propname.GetChars(), pcls->TypeName.GetChars());
		}
	}
	else
	{
		propname.Insert(0, "@property@");
		FName name(propname, true);
		if (name != NAME_None)
		{
			auto propp = dyn_cast<PProperty>(cls->FindSymbol(name, true));
			if (propp != nullptr)
			{
				DispatchScriptProperty(propp, prop, (DCoreActor *)bag.Info->Defaults, bag);
				return;
			}
		}
		Error(prop, "'%s' is an unknown actor property\n", propname.GetChars());
	}
}

//==========================================================================
//
// Finds a flag and sets or clears it
//
//==========================================================================

void ZCCRazeCompiler::ProcessDefaultFlag(PClassActor *cls, ZCC_FlagStmt *flg)
{
	auto namenode = flg->name;
	const char *n1 = FName(namenode->Id).GetChars(), *n2;

	if (namenode->SiblingNext == namenode)
	{
		// a one-name flag
		n2 = nullptr;
	}
	else if (namenode->SiblingNext->SiblingNext == namenode)
	{
		// a two-name flag
		n2 = FName(static_cast<ZCC_Identifier *>(namenode->SiblingNext)->Id).GetChars();
	}
	else
	{
		Error(flg, "Flag name may at most contain two parts");
		return;
	}

	auto fd = FindFlag(cls, n1, n2, true);
	if (fd != nullptr)
	{
		if (fd->varflags & VARF_Deprecated)
		{
			Warn(flg, "Deprecated flag '%s%s%s' used", n1, n2 ? "." : "", n2 ? n2 : "");
		}
		if (fd->structoffset == -1)
		{
			HandleDeprecatedFlags((DCoreActor*)cls->Defaults, cls, flg->set, fd->flagbit);
		}
		else
		{
			ModActorFlag((DCoreActor*)cls->Defaults, fd, flg->set);
		}
	}
	else
	{
		Error(flg, "Unknown flag '%s%s%s'", n1, n2 ? "." : "", n2 ? n2 : "");
	}
}

//==========================================================================
//
// Parses the default list
//
//==========================================================================

void ZCCRazeCompiler::InitDefaults()
{
	for (auto c : Classes)
	{
		// This may be removed if the conditions change, but right now only subclasses of Actor can define a Default block.
		if (!c->ClassType()->IsDescendantOf(RUNTIME_CLASS(DCoreActor)))
		{
			if (c->Defaults.Size()) Error(c->cls, "%s: Non-actor classes may not have defaults", c->ClassType()->TypeName.GetChars());
			if (c->ClassType()->ParentClass)
			{
				auto ti = c->ClassType();
				ti->InitializeDefaults();
			}
		}
		else
		{
			auto cls = c->ClassType();
			// This should never happen.
			if (cls->Defaults != nullptr)
			{
				Error(c->cls, "%s already has defaults", cls->TypeName.GetChars());
			}
			// This can only occur if a native parent is not initialized. In all other cases the sorting of the class list should prevent this from ever happening.
			else if (cls->ParentClass->Defaults == nullptr && cls != RUNTIME_CLASS(DCoreActor))
			{
				Error(c->cls, "Parent class %s of %s is not initialized", cls->ParentClass->TypeName.GetChars(), cls->TypeName.GetChars());
			}
			else
			{
				// Copy the parent's defaults and meta data.
				auto ti = static_cast<PClassActor *>(cls);

				ti->InitializeDefaults();

				// Replacements require that the meta data has been allocated by InitializeDefaults.
				if (c->cls->Replaces != nullptr && !ti->SetReplacement(c->cls->Replaces->Id))
				{
					Warn(c->cls, "Replaced type '%s' not found for %s", FName(c->cls->Replaces->Id).GetChars(), ti->TypeName.GetChars());
				}


				Baggage bag;
				bag.Version = mVersion;
				bag.Namespace = OutNamespace;
				bag.Info = ti;
				bag.Lumpnum = c->cls->SourceLump;
				// The actual script position needs to be set per property.

				for (auto d : c->Defaults)
				{
					auto content = d->Content;
					if (content != nullptr) do
					{
						switch (content->NodeType)
						{
						case AST_PropertyStmt:
							bag.ScriptPosition.FileName = *content->SourceName;
							bag.ScriptPosition.ScriptLine = content->SourceLoc;
							ProcessDefaultProperty(ti, static_cast<ZCC_PropertyStmt *>(content), bag);
							break;

						case AST_FlagStmt:
							ProcessDefaultFlag(ti, static_cast<ZCC_FlagStmt *>(content));
							break;

						default:
							break;
						}
						content = static_cast<decltype(content)>(content->SiblingNext);
					} while (content != d->Content);
				}
			}
		}
	}
}

//==========================================================================
//
// DCoreActor needs the actor info manually added to its meta data 
// before adding any scripted fields.
//
//==========================================================================

bool ZCCRazeCompiler::PrepareMetaData(PClass *type)
{
	if (type == RUNTIME_CLASS(DCoreActor))
	{
		assert(type->MetaSize == 0);
		AddActorInfo(type);	
		return true;
	}
	return false;
}

