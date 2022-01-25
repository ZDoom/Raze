//-----------------------------------------------------------------------------
//
// Copyright 2016-2022 Christoph Oelckers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//-----------------------------------------------------------------------------
//
// VM iterators
//
// These classes are thin wrappers which wrap the standard iterators into a DObject
// so that the VM can use them
//
//-----------------------------------------------------------------------------

BEGIN_DUKE_NS

//==========================================================================
//
// scriptable stat iterator
//
//==========================================================================

class DDukeStatIterator : public DObject
{
	DECLARE_ABSTRACT_CLASS(DDukeStatIterator, DObject)

public:
	
	DukeStatIterator it;

	DDukeStatIterator(int statnum)
		: it(statnum)
	{
	}
};

IMPLEMENT_CLASS(DDukeStatIterator, true, false);

static DDukeStatIterator *CreateStatIterator(int statnum)
{
	return Create<DDukeStatIterator>(statnum);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, CreateStatIterator, CreateStatIterator)
{
	PARAM_PROLOGUE;
	PARAM_INT(statnum);
	ACTION_RETURN_OBJECT(CreateStatIterator(statnum));
}

static DDukeActor *NextStat(DDukeStatIterator *self)
{
	return self->it.Next();
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeStatIterator, Next, NextStat)
{
	PARAM_SELF_PROLOGUE(DDukeStatIterator);
	ACTION_RETURN_OBJECT(NextStat(self));
}

static void ResetStat(DDukeStatIterator *self, int stat)
{
	self->it.Reset(stat);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeStatIterator, Reset, ResetStat)
{
	PARAM_SELF_PROLOGUE(DDukeStatIterator);
	PARAM_INT(stat);
	ResetStat(self, stat);
	return 0;
}

//==========================================================================
//
// scriptable sector iterator
//
//==========================================================================

class DDukeSectIterator : public DObject
{
	DECLARE_ABSTRACT_CLASS(DDukeSectIterator, DObject)

public:
	
	DukeSectIterator it;

	DDukeSectIterator(int Sectnum)
		: it(Sectnum)
	{
	}
};

IMPLEMENT_CLASS(DDukeSectIterator, true, false);

static DDukeSectIterator *CreateSectIterator(int Sectnum)
{
	return Create<DDukeSectIterator>(Sectnum);
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, CreateSectorIterator, CreateSectIterator)
{
	PARAM_PROLOGUE;
	PARAM_INT(Sectnum);
	ACTION_RETURN_OBJECT(CreateSectIterator(Sectnum));
}

static DDukeActor *NextSect(DDukeSectIterator *self)
{
	return self->it.Next();
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeSectIterator, Next, NextSect)
{
	PARAM_SELF_PROLOGUE(DDukeSectIterator);
	ACTION_RETURN_OBJECT(NextSect(self));
}

static void ResetSect(DDukeSectIterator *self, int Sect)
{
	self->it.Reset(Sect);
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeSectIterator, Reset, ResetSect)
{
	PARAM_SELF_PROLOGUE(DDukeSectIterator);
	PARAM_INT(Sect);
	ResetSect(self, Sect);
	return 0;
}

//==========================================================================
//
// scriptable sprite iterator
//
//==========================================================================

class DDukeSpriteIterator : public DObject
{
	DECLARE_ABSTRACT_CLASS(DDukeSpriteIterator, DObject)

public:
	
	DukeSpriteIterator it;
};

IMPLEMENT_CLASS(DDukeSpriteIterator, true, false);

static DDukeSpriteIterator *CreateSpriteIterator()
{
	return Create<DDukeSpriteIterator>();
}

DEFINE_ACTION_FUNCTION_NATIVE(_DukeLevel, CreateSpriteIterator, CreateSpriteIterator)
{
	PARAM_PROLOGUE;
	ACTION_RETURN_OBJECT(CreateSpriteIterator());
}

static DDukeActor *NextSprite(DDukeSpriteIterator *self)
{
	return self->it.Next();
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeSpriteIterator, Next, NextSprite)
{
	PARAM_SELF_PROLOGUE(DDukeSpriteIterator);
	ACTION_RETURN_OBJECT(NextSprite(self));
}

static void ResetSprite(DDukeSpriteIterator *self)
{
	self->it.Reset();
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeSpriteIterator, Reset, ResetSprite)
{
	PARAM_SELF_PROLOGUE(DDukeSpriteIterator);
	ResetSprite(self);
	return 0;
}

END_DUKE_NS
