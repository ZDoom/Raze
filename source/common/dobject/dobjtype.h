#ifndef DOBJTYPE_H
#define DOBJTYPE_H

#ifndef __DOBJECT_H__
#error You must #include "dobject.h" to get dobjtype.h
#endif

#include "name.h"


// Meta-info for every class derived from DObject ---------------------------

struct PClass
{
	static void StaticInit ();

	// Per-class information -------------------------------------
	FName				 TypeName;		// this class's name
	unsigned int		 Size;			// this class's size
	PClass				*ParentClass;	// the class this class derives from
	PClass				*HashNext;

	void (*ConstructNative)(void *);

	// The rest are all functions and static data ----------------
	DObject *CreateNew () const;

	// Returns true if this type is an ancestor of (or same as) the passed type.
	bool IsAncestorOf (const PClass *ti) const
	{
		while (ti)
		{
			if (this == ti)
				return true;
			ti = ti->ParentClass;
		}
		return false;
	}
	inline bool IsDescendantOf (const PClass *ti) const
	{
		return ti->IsAncestorOf (this);
	}

	// Find a type, given its name.
	static const PClass *FindClass (const char *name) { return FindClass (FName (name, true)); }
	static const PClass *FindClass (const FString &name) { return FindClass (FName (name, true)); }
	static const PClass *FindClass (ENamedName name) { return FindClass (FName (name)); }
	static const PClass *FindClass (FName name);
};

#endif
