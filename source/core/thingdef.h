/*
** thingdef.h
**
** Actor definitions
**
**---------------------------------------------------------------------------
** Copyright 2002-2008 Christoph Oelckers
** Copyright 2004-2008 Randy Heit
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
** 4. When not used as part of ZDoom or a ZDoom derivative, this code will be
**    covered by the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or (at
**    your option) any later version.
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


#ifndef __THINGDEF_H
#define __THINGDEF_H

#include "sc_man.h"
#include "cmdlib.h"
#include "vm.h"
#include "dobject.h"
#include "coreactor.h"


class FScanner;


//==========================================================================
//
// A flag descriptor
//
//==========================================================================

struct FFlagDef
{
	unsigned int flagbit;
	const char *name;
	int structoffset;
	int fieldsize;
	int varflags;
};

void FinalizeClass(PClass *cls);
FFlagDef *FindFlag (const PClass *type, const char *part1, const char *part2, bool strict = false);
const char* GetFlagName(unsigned int flagnum, int flagoffset);
void HandleDeprecatedFlags(DCoreActor *defaults, PClassActor *info, bool set, int index);
bool CheckDeprecatedFlags(DCoreActor *actor, PClassActor *info, int index);
void ModActorFlag(DCoreActor *actor, FFlagDef *fd, bool set);
bool ModActorFlag(DCoreActor *actor, const FString &flagname, bool set, bool printerror = true);
INTBOOL CheckActorFlag(DCoreActor *actor, FFlagDef *fd);
INTBOOL CheckActorFlag(DCoreActor *owner, const char *flagname, bool printerror = true);

#define FLAG_NAME(flagnum, flagvar) GetFlagName(flagnum, myoffsetof(DCoreActor, flagvar))


//==========================================================================
//
// State parser
//
//==========================================================================
class FxExpression;

//==========================================================================
//
// Extra info maintained while defining an actor.
//
//==========================================================================
struct FDropItem;

struct Baggage
{
	PNamespace *Namespace;
	PClassActor *Info;
	int Lumpnum;
	VersionInfo Version;

	FScriptPosition ScriptPosition;
};

inline void ResetBaggage (Baggage *bag, PClassActor *stateclass)
{
}

//==========================================================================
//
// Property parser
//
//==========================================================================

void HandleActorFlag(FScanner &sc, Baggage &bag, const char *part1, const char *part2, int mod);
FxExpression *ParseParameter(FScanner &sc, PClassActor *cls, PType *type);


enum 
{
	DEPF_UNUSED = 0,
	DEPF_FIREDAMAGE = 1,
	DEPF_ICEDAMAGE = 2,
	DEPF_LOWGRAVITY = 3,
	DEPF_LONGMELEERANGE = 4,
	DEPF_SHORTMISSILERANGE = 5,
	DEPF_PICKUPFLASH = 6,
	DEPF_QUARTERGRAVITY = 7,
	DEPF_FIRERESIST = 8,
	DEPF_HERETICBOUNCE = 9,
	DEPF_HEXENBOUNCE = 10,
	DEPF_DOOMBOUNCE = 11,
	DEPF_INTERHUBSTRIP = 12,
	DEPF_HIGHERMPROB = 13,
};

// Types of old style decorations
enum EDefinitionType
{
	DEF_Decoration,
	DEF_BreakableDecoration,
	DEF_Pickup,
	DEF_Projectile,
};

#if defined(_MSC_VER)
#pragma section(SECTION_GREG,read)

#define MSVC_PSEG __declspec(allocate(SECTION_GREG))
#define GCC_PSEG
#else
#define MSVC_PSEG
#define GCC_PSEG __attribute__((section(SECTION_GREG))) __attribute__((used))
#endif


union FPropParam
{
	int i;
	double d;
	const char *s;
	FxExpression *exp;
};

typedef void (*PropHandler)(DCoreActor *defaults, PClassActor *info, Baggage &bag, FPropParam *params);

enum ECategory
{
	CAT_PROPERTY,	// Inheritable property
	CAT_INFO		// non-inheritable info (spawn ID, Doomednum, game filter, conversation ID, not usable in ZScript)
};

struct FPropertyInfo
{
	const char *name;
	const char *params;
	const char *clsname;
	PropHandler Handler;
	int category;
};

FPropertyInfo *FindProperty(const char * string);
int MatchString (const char *in, const char **strings);


#define DEFINE_PROPERTY_BASE(name, paramlist, clas, cat) \
	static void Handler_##name##_##paramlist##_##clas(D##clas *defaults, PClassActor *info, Baggage &bag, FPropParam *params); \
	static FPropertyInfo Prop_##name##_##paramlist##_##clas = \
		{ #name, #paramlist, #clas, (PropHandler)Handler_##name##_##paramlist##_##clas, cat }; \
	MSVC_PSEG FPropertyInfo *infoptr_##name##_##paramlist##_##clas GCC_PSEG = &Prop_##name##_##paramlist##_##clas; \
	static void Handler_##name##_##paramlist##_##clas(D##clas *defaults, PClassActor *info, Baggage &bag, FPropParam *params)

#define DEFINE_PREFIXED_PROPERTY_BASE(prefix, name, paramlist, clas, cat) \
	static void Handler_##name##_##paramlist##_##clas(D##clas *defaults, PClassActor *info, Baggage &bag, FPropParam *params); \
	static FPropertyInfo Prop_##name##_##paramlist##_##clas = \
{ #prefix"."#name, #paramlist, #clas, (PropHandler)Handler_##name##_##paramlist##_##clas, cat }; \
	MSVC_PSEG FPropertyInfo *infoptr_##name##_##paramlist##_##clas GCC_PSEG = &Prop_##name##_##paramlist##_##clas; \
	static void Handler_##name##_##paramlist##_##clas(D##clas *defaults, PClassActor *info, Baggage &bag, FPropParam *params)

#define DEFINE_PREFIXED_SCRIPTED_PROPERTY_BASE(prefix, name, paramlist, clas, cat) \
	static void Handler_##name##_##paramlist##_##clas(DCoreActor *defaults, PClassActor *info, Baggage &bag, FPropParam *params); \
	static FPropertyInfo Prop_##name##_##paramlist##_##clas = \
{ #prefix"."#name, #paramlist, #clas, (PropHandler)Handler_##name##_##paramlist##_##clas, cat }; \
	MSVC_PSEG FPropertyInfo *infoptr_##name##_##paramlist##_##clas GCC_PSEG = &Prop_##name##_##paramlist##_##clas; \
	static void Handler_##name##_##paramlist##_##clas(DCoreActor *defaults, PClassActor *info, Baggage &bag, FPropParam *params)


#define DEFINE_PROPERTY(name, paramlist, clas) DEFINE_PROPERTY_BASE(name, paramlist, clas, CAT_PROPERTY)
#define DEFINE_INFO_PROPERTY(name, paramlist, clas) DEFINE_PROPERTY_BASE(name, paramlist, clas, CAT_INFO)

#define DEFINE_CLASS_PROPERTY(name, paramlist, clas) DEFINE_PREFIXED_SCRIPTED_PROPERTY_BASE(clas, name, paramlist, clas, CAT_PROPERTY)
#define DEFINE_CLASS_PROPERTY_PREFIX(prefix, name, paramlist, clas) DEFINE_PREFIXED_SCRIPTED_PROPERTY_BASE(prefix, name, paramlist, clas, CAT_PROPERTY)

#define PROP_PARM_COUNT (params[0].i)

#define PROP_STRING_PARM(var, no) \
	const char *var = params[(no)+1].s;

#define PROP_EXP_PARM(var, no) \
	FxExpression *var = params[(no)+1].exp;

#define PROP_INT_PARM(var, no) \
	int var = params[(no)+1].i;

#define PROP_FLOAT_PARM(var, no) \
	float var = float(params[(no)+1].d);

#define PROP_DOUBLE_PARM(var, no) \
	double var = params[(no)+1].d;

#define PROP_COLOR_PARM(var, no, scriptpos) \
	int var = params[(no)+1].i== 0? params[(no)+2].i : V_GetColor(params[(no)+2].s, scriptpos);

#endif
