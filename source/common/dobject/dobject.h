/*
** dobject.h
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

#ifndef __DOBJECT_H__
#define __DOBJECT_H__

#include <stdlib.h>

struct PClass;

#define RUNTIME_TYPE(object)	(object->GetClass())	// Passed an object, returns the type of that object
#define RUNTIME_CLASS(cls)		(&cls::_StaticType)		// Passed a class name, returns a PClass representing that class
#define NATIVE_TYPE(object)		(object->StaticType())	// Passed an object, returns the type of the C++ class representing the object

struct ClassReg
{
	PClass *MyClass;
	const char *Name;
	PClass *ParentType;
	unsigned int SizeOf;
	const size_t *Pointers;
	void (*ConstructNative)(void *);

	void RegisterClass() const;
};

enum EInPlace { EC_InPlace };

#define DECLARE_ABSTRACT_CLASS(cls,parent) \
public: \
	static PClass _StaticType; \
	virtual PClass *StaticType() const { return &_StaticType; } \
	static ClassReg RegistrationInfo, *RegistrationInfoPtr; \
private: \
	typedef parent Super; \
	typedef cls ThisClass;

#define DECLARE_CLASS(cls,parent) \
	DECLARE_ABSTRACT_CLASS(cls,parent) \
		private: static void InPlaceConstructor (void *mem);

#if defined(_MSC_VER)
#	pragma data_seg(".creg$u")
#	pragma data_seg()
#	define _DECLARE_TI(cls) __declspec(allocate(".creg$u")) ClassReg *cls::RegistrationInfoPtr = &cls::RegistrationInfo;
#else
#	define _DECLARE_TI(cls) ClassReg *cls::RegistrationInfoPtr __attribute__((section(SECTION_CREG))) = &cls::RegistrationInfo;
#endif

#define _IMP_PCLASS(cls,ptrs,create) \
	PClass cls::_StaticType; \
	ClassReg cls::RegistrationInfo = {\
		RUNTIME_CLASS(cls), \
		#cls, \
		RUNTIME_CLASS(cls::Super), \
		sizeof(cls), \
		ptrs, \
		create }; \
	_DECLARE_TI(cls)

#define _IMP_CREATE_OBJ(cls) \
	void cls::InPlaceConstructor(void *mem) { new((EInPlace *)mem) cls; }

#define IMPLEMENT_CLASS(cls) \
	_IMP_CREATE_OBJ(cls) \
	_IMP_PCLASS(cls,NULL,cls::InPlaceConstructor) 

#define IMPLEMENT_ABSTRACT_CLASS(cls) \
	_IMP_PCLASS(cls,NULL,NULL)


class DObject
{
public:
	static PClass _StaticType;
	virtual PClass *StaticType() const { return &_StaticType; }
	static ClassReg RegistrationInfo, *RegistrationInfoPtr;
	static void InPlaceConstructor (void *mem);
private:
	typedef DObject ThisClass;

	// Per-instance variables. There are four.
private:
	PClass *Class;				// This object's type
public:

public:
	DObject ();
	DObject (PClass *inClass);
	virtual ~DObject();

	inline bool IsKindOf (const PClass *base);
	inline bool IsA (const PClass *type);

	virtual void Destroy ();

	PClass *GetClass() const
	{
		if (Class == NULL)
		{
			// Save a little time the next time somebody wants this object's type
			// by recording it now.
			const_cast<DObject *>(this)->Class = StaticType();
		}
		return Class;
	}

	void SetClass (PClass *inClass)
	{
		Class = inClass;
	}

protected:
	// This form of placement new and delete is for use *only* by PClass's
	// CreateNew() method. Do not use them for some other purpose.
	void *operator new(size_t, EInPlace *mem)
	{
		return (void *)mem;
	}

	void operator delete (void *mem, EInPlace *)
	{
		free (mem);
	}
};

#include "dobjtype.h"

inline bool DObject::IsKindOf (const PClass *base)
{
	return base->IsAncestorOf (GetClass ());
}

inline bool DObject::IsA (const PClass *type)
{
	return (type == GetClass());
}

#endif //__DOBJECT_H__
