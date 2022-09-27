/*
** fixedhorizon.h
**
** type safe representations of high precision horizon values. 
**
**---------------------------------------------------------------------------
** Copyright 2020-2022 Christoph Oelckers
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

#pragma once

#include <math.h>
#include "basics.h"
#include "m_fixed.h"
#include "vectors.h"
#include "xs_Float.h"	// needed for reliably overflowing float->int conversions.
#include "serializer.h"
#include "math/cmath.h"

class FSerializer;

//---------------------------------------------------------------------------
//
// Functions for use with fixedhoriz and friendly functions.
//
//---------------------------------------------------------------------------

inline double HorizToPitch(double horiz) { return atan2(horiz, 128) * (180. / pi::pi()); }
inline double HorizToPitch(fixed_t q16horiz) { return atan2(q16horiz, IntToFixed(128)) * (180. / pi::pi()); }


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class fixedhoriz
{
	fixed_t value;

	constexpr fixedhoriz(fixed_t v) : value(v) {}

	friend constexpr fixedhoriz q16horiz(fixed_t v);
	friend constexpr fixedhoriz buildhoriz(int v);
	friend constexpr fixedhoriz tanhoriz(double v);
	friend fixedhoriz pitchhoriz(double v);

	friend FSerializer &Serialize(FSerializer &arc, const char *key, fixedhoriz &obj, fixedhoriz *defval);

public:
	fixedhoriz() = default;
	fixedhoriz(const fixedhoriz &other) = default;
	fixedhoriz& operator=(const fixedhoriz&) = default;

	// This class intentionally makes no allowances for implicit type conversions because those would render it ineffective.
	constexpr short asbuild() const { return FixedToInt(value); }
	constexpr double Tan() const { return FixedToFloat<23>(value); }
	double Degrees() const { return HorizToPitch(value); }

	int Sgn() const { return ::Sgn(value); }

	bool operator< (fixedhoriz other) const
	{
		return value < other.value;
	}

	bool operator> (fixedhoriz other) const
	{
		return value > other.value;
	}

	bool operator<= (fixedhoriz other) const
	{
		return value <= other.value;
	}

	bool operator>= (fixedhoriz other) const
	{
		return value >= other.value;
	}
	constexpr bool operator== (fixedhoriz other) const
	{
		return value == other.value;
	}

	constexpr bool operator!= (fixedhoriz other) const
	{
		return value != other.value;
	}

	constexpr fixedhoriz &operator+= (fixedhoriz other)
	{
		value += other.value;
		return *this;
	}

	constexpr fixedhoriz &operator-= (fixedhoriz other)
	{
		value -= other.value;
		return *this;
	}

	constexpr fixedhoriz operator- () const
	{
		return fixedhoriz(-value);
	}

	constexpr fixedhoriz operator+ (fixedhoriz other) const
	{
		return fixedhoriz(value + other.value);
	}

	constexpr fixedhoriz operator- (fixedhoriz other) const
	{
		return fixedhoriz(value - other.value);
	}

	constexpr fixedhoriz &operator<<= (const uint8_t shift)
	{
		value <<= shift;
		return *this;
	}

	constexpr fixedhoriz &operator>>= (const uint8_t shift)
	{
		value >>= shift;
		return *this;
	}

	constexpr fixedhoriz operator<< (const uint8_t shift) const
	{
		return fixedhoriz(value << shift);
	}

	constexpr fixedhoriz operator>> (const uint8_t shift) const
	{
		return fixedhoriz(value >> shift);
	}

	template<class T>
	constexpr fixedhoriz &operator*= (const T other)
	{
		value = value * other;
		return *this;
	}

	template<class T>
	constexpr fixedhoriz operator* (const T other) const
	{
		return value * other;
	}
};

inline constexpr fixedhoriz q16horiz(fixed_t v) { return fixedhoriz(v); }
inline constexpr fixedhoriz buildhoriz(int v) { return fixedhoriz(IntToFixed(v)); }
inline constexpr fixedhoriz tanhoriz(double v) { return fixedhoriz(FloatToFixed<23>(v)); }
inline fixedhoriz pitchhoriz(double v) { return fixedhoriz(fixed_t(clamp<double>(IntToFixed(128) * tan(v * (pi::pi() / 180.)), -INT32_MAX, INT32_MAX))); }

inline FSerializer &Serialize(FSerializer &arc, const char *key, fixedhoriz &obj, fixedhoriz *defval)
{
	return Serialize(arc, key, obj.value, defval ? &defval->value : nullptr);
}
