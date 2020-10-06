/*
** binaryangle.h
**
** type safe representations of high precision angle and horizon values. 
** Angle uses natural 32 bit overflow to clamp to one rotation.
**
**---------------------------------------------------------------------------
** Copyright 2020 Christoph Oelckers
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
#include "m_fixed.h"
#include "xs_Float.h"	// needed for reliably overflowing float->int conversions.
#include "build.h"


class binangle
{
	unsigned int value;
	
	inline static constexpr double pi() { return 3.14159265358979323846; }
	
	constexpr binangle(unsigned int v) : value(v) {}
	
	friend constexpr binangle bamang(unsigned int v);
	friend constexpr binangle q16ang(unsigned int v);
	friend constexpr binangle buildang(unsigned int v);
	friend binangle radang(double v);
	friend binangle degang(double v);
	
public:
	binangle() = default;
	binangle(const binangle &other) = default;
	// This class intentionally makes no allowances for implicit type conversions because those would render it ineffective.
	constexpr short asbuild() const { return value >> 21; }
	constexpr fixed_t asq16() const { return value >> 5; }
	constexpr double asrad() const { return value * (pi::pi() / 0x80000000u); }
	constexpr double asdeg() const { return value * (90. / 0x40000000); }
	constexpr unsigned asbam() const { return value; }
	
	double fsin() const { return sin(asrad()); }
	double fcos() const { return cos(asrad()); }
	double ftan() const { return tan(asrad()); }
	int bsin() const { return sintable[asbuild()]; }
	int bcos() const { return sintable[(asbuild() + 512) & 2047]; }
	
#if 0 // This makes no sense
	bool operator< (binangle other) const
	{
		return value < other.value;
	}

	bool operator> (binangle other) const
	{
		return value > other.value;
	}

	bool operator<= (binangle other) const
	{
		return value <= other.value;
	}

	bool operator>= (binangle other) const
	{
		return value >= other.value;
	}
#endif
	constexpr bool operator== (binangle other) const
	{
		return value == other.value;
	}

	constexpr bool operator!= (binangle other) const
	{
		return value != other.value;
	}

	constexpr binangle &operator+= (binangle other)
	{
		value += other.value;
		return *this;
	}

	constexpr binangle &operator-= (binangle other)
	{
		value -= other.value;
		return *this;
	}

	constexpr binangle operator+ (binangle other) const
	{
		return binangle(value + other.value);
	}

	constexpr binangle operator- (binangle other) const
	{
		return binangle(value - other.value);
	}
	
	void interpolate(binangle a1, binangle a2, fixed_t smoothratio)
	{
		// Calculate in floating point to reduce the error caused by overflows which are to be expected here and then downconvert using a method that is safe to overflow.
		// We do not want fixed point multiplications here to trash the result.
		double smooth = smoothratio / 65536.f;
		value = xs_CRoundToUInt(double(a1.asbam()) + smooth * (double(a2.asbam()) - double(a1.asbam())));
	}


};

//---------------------------------------------------------------------------
//
// Constants and functions for use with fixedhoriz and friendly functions.
//
//---------------------------------------------------------------------------

// 280039127 is the maximum horizon in Q16.16 the engine will handle before wrapping around.
constexpr double horizDiff = 280039127 * 3. / 100.;

// Degrees needed to convert horizAngle into pitch degrees.
constexpr double horizDegrees = 183.503609961216825;

// Ratio to convert inverse tangent to -90/90 degrees of pitch.
constexpr double horizRatio = horizDegrees / pi::pi();

// Horizon conversion functions.
inline double HorizToPitch(double horiz) { return atan2(horiz, horizDiff / 65536.) * horizRatio; }
inline double HorizToPitch(fixed_t q16horiz) { return atan2(q16horiz, horizDiff) * horizRatio; }
inline fixed_t PitchToHoriz(double horizAngle) { return xs_CRoundToInt(horizDiff * tan(horizAngle * (pi::pi() / horizDegrees))); }
inline int32_t PitchToBAM(double horizAngle) { return xs_CRoundToInt(clamp(horizAngle * (1073741823.5 / 45.), -INT32_MAX, INT32_MAX)); }
inline constexpr double BAMToPitch(int32_t bam) { return bam * (45. / 1073741823.5); }


class fixedhoriz
{
	fixed_t value;
	
	constexpr fixedhoriz(fixed_t v) : value(v) {}
	
	friend constexpr fixedhoriz q16horiz(fixed_t v);
	friend constexpr fixedhoriz buildhoriz(int v);
	friend fixedhoriz pitchhoriz(double v);
	friend fixedhoriz bamhoriz(int32_t v);
	
public:
	fixedhoriz() = default;
	fixedhoriz(const fixedhoriz &other) = default;

	// This class intentionally makes no allowances for implicit type conversions because those would render it ineffective.
	constexpr short asbuild() const { return FixedToInt(value); }
	constexpr fixed_t asq16() const { return value; }
	double aspitch() const { return HorizToPitch(value); }
	int32_t asbam() const { return PitchToBAM(aspitch()); }
	
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

};


inline constexpr binangle bamang(unsigned int v) { return binangle(v); }
inline constexpr binangle q16ang(unsigned int v) { return binangle(v << 5); }
inline constexpr binangle buildang(unsigned int v) { return binangle(v << 21); }
inline binangle radang(double v) { return binangle(xs_CRoundToUInt(v * (0x80000000u / binangle::pi()))); }
inline binangle degang(double v) { return binangle(xs_CRoundToUInt(v * (0x40000000 / 90.))); }

inline constexpr fixedhoriz q16horiz(fixed_t v) { return fixedhoriz(v); }
inline constexpr fixedhoriz buildhoriz(int v) { return fixedhoriz(IntToFixed(v)); }
inline fixedhoriz pitchhoriz(double v) { return fixedhoriz(PitchToHoriz(v)); }
inline fixedhoriz bamhoriz(int32_t v) { return pitchhoriz(BAMToPitch(v)); }

