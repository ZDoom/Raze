/*
** dobjtype.cpp
** Implements the type information class
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
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
#include <stdint.h>
#include "dobject.h"
#include "templates.h"
#include "autosegs.h"
#include "tarray.h"
#if 0

static TArray<PClass *> Types;
static TMap<FName, PClass *> Map;

static int cregcmp (const void *a, const void *b)
{
	// VC++ introduces NULLs in the sequence. GCC seems to work as expected and not do it.
	const ClassReg *class1 = *(const ClassReg **)a;
	const ClassReg *class2 = *(const ClassReg **)b;
	if (class1 == NULL) return 1;
	if (class2 == NULL) return -1;
	return strcmp (class1->Name, class2->Name);
}

void PClass::StaticInit ()
{
	// Sort classes by name to remove dependance on how the compiler ordered them.
	REGINFO *head = &CRegHead;
	REGINFO *tail = &CRegTail;

	// MinGW's linker is linking the object files backwards for me now...
	if (head > tail)
	{
		std::swap (head, tail);
	}
	qsort ((void*)(head + 1), tail - head - 1, sizeof(REGINFO), cregcmp);

	FAutoSegIterator probe(CRegHead, CRegTail);

	while (*++probe != NULL)
	{
		((ClassReg *)*probe)->RegisterClass ();
	}
}

void ClassReg::RegisterClass () const
{
	assert (MyClass != NULL);

	// Add type to list
	MyClass->TypeName = FName(Name+1);
	MyClass->ParentClass = ParentType;
	MyClass->Size = SizeOf;
	MyClass->ConstructNative = ConstructNative;
	Map.Insert(MyClass->TypeName, MyClass);
}

// Find a type, passed the name as a name
const PClass *PClass::FindClass (FName zaname)
{
	auto pcls = Map.CheckKey(zaname);
	return pcls? *pcls : nullptr;
}

// Create a new object that this class represents
DObject *PClass::CreateNew () const
{
	uint8_t *mem = (uint8_t *)calloc (Size, 1);
	assert (mem != NULL);

	ConstructNative (mem);
	((DObject *)mem)->SetClass (const_cast<PClass *>(this));
	return (DObject *)mem;
}
#endif