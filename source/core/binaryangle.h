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
#include "serializer.h"
#include "build.h"
#include "math/cmath.h"

class FSerializer;

enum
{
	BAMBITS = 21,
	BAMUNIT = 1 << BAMBITS,
	SINSHIFT = 14
};

//---------------------------------------------------------------------------
//
// Constants used for Build sine/cosine functions.
//
//---------------------------------------------------------------------------

constexpr double BAngRadian = pi::pi() * (1. / 1024.);
constexpr double BRadAngScale = 1. / BAngRadian;


//---------------------------------------------------------------------------
//
// Build sine inline functions.
//
//---------------------------------------------------------------------------

inline int32_t bsin(const int ang, const int8_t shift = 0)
{
	return shift < 0 ? sintable[ang & 2047] >> abs(shift) : sintable[ang & 2047] << shift;
}
inline double bsinf(const double ang, const int8_t shift = 0)
{
	return g_sin(ang * BAngRadian) * (shift >= -SINSHIFT ? uint64_t(1) << (SINSHIFT + shift) : 1. / (uint64_t(1) << abs(SINSHIFT + shift)));
}


//---------------------------------------------------------------------------
//
// Build cosine inline functions.
//
//---------------------------------------------------------------------------

inline int32_t bcos(const int ang, const int8_t shift = 0)
{
	return shift < 0 ? sintable[(ang + 512) & 2047] >> abs(shift) : sintable[(ang + 512) & 2047] << shift;
}
inline double bcosf(const double ang, const int8_t shift = 0)
{
	return g_cos(ang * BAngRadian) * (shift >= -SINSHIFT ? uint64_t(1) << (SINSHIFT + shift) : 1. / (uint64_t(1) << abs(SINSHIFT + shift)));
}


//---------------------------------------------------------------------------
//
// Shift a Build angle left by 21 bits.
//
//---------------------------------------------------------------------------

inline constexpr int64_t BAngToBAM(int ang)
{
	return ang << BAMBITS;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class lookangle
{
	int32_t value;
	
	constexpr lookangle(int32_t v) : value(v) {}
	
	friend constexpr lookangle bamlook(int32_t v);
	friend constexpr lookangle q16look(int32_t v);
	friend constexpr lookangle buildlook(int32_t v);
	friend lookangle radlook(double v);
	friend lookangle deglook(double v);

	friend FSerializer &Serialize(FSerializer &arc, const char *key, lookangle &obj, lookangle *defval);

	friend class binangle;
	
public:
	lookangle() = default;
	lookangle(const lookangle &other) = default;
	// This class intentionally makes no allowances for implicit type conversions because those would render it ineffective.
	constexpr short asbuild() const { return value >> 21; }
	constexpr double asbuildf() const { return value * (1. / BAMUNIT); }
	constexpr fixed_t asq16() const { return value >> 5; }
	constexpr double asrad() const { return value * (pi::pi() / 0x80000000u); }
	constexpr double asdeg() const { return AngleToFloat(value); }
	constexpr int32_t asbam() const { return value; }
	
	double fsin() const { return g_sin(asrad()); }
	double fcos() const { return g_cos(asrad()); }
	double ftan() const { return g_tan(asrad()); }
	int bsin(const int8_t& shift = 0) const { return ::bsin(asbuild(), shift); }
	int bcos(const int8_t& shift = 0) const { return ::bcos(asbuild(), shift); }

	bool operator< (lookangle other) const
	{
		return value < other.value;
	}

	bool operator> (lookangle other) const
	{
		return value > other.value;
	}

	bool operator<= (lookangle other) const
	{
		return value <= other.value;
	}

	bool operator>= (lookangle other) const
	{
		return value >= other.value;
	}
	constexpr bool operator== (lookangle other) const
	{
		return value == other.value;
	}

	constexpr bool operator!= (lookangle other) const
	{
		return value != other.value;
	}

	constexpr lookangle &operator+= (lookangle other)
	{
		value += other.value;
		return *this;
	}

	constexpr lookangle &operator-= (lookangle other)
	{
		value -= other.value;
		return *this;
	}

	constexpr lookangle operator+ (lookangle other) const
	{
		return lookangle(value + other.value);
	}

	constexpr lookangle operator- (lookangle other) const
	{
		return lookangle(value - other.value);
	}

	constexpr lookangle &operator<<= (const uint8_t shift)
	{
		value <<= shift;
		return *this;
	}

	constexpr lookangle &operator>>= (const uint8_t shift)
	{
		value >>= shift;
		return *this;
	}

	constexpr lookangle operator<< (const uint8_t shift) const
	{
		return lookangle(value << shift);
	}

	constexpr lookangle operator>> (const uint8_t shift) const
	{
		return lookangle(value >> shift);
	}

};

inline constexpr lookangle bamlook(int32_t v) { return lookangle(v); }
inline constexpr lookangle q16look(int32_t v) { return lookangle(v << 5); }
inline constexpr lookangle buildlook(int32_t v) { return lookangle(v << BAMBITS); }
inline lookangle radlook(double v) { return lookangle(xs_CRoundToUInt(v * (0x80000000u / pi::pi()))); }
inline lookangle deglook(double v) { return lookangle(FloatToAngle(v)); }

inline FSerializer &Serialize(FSerializer &arc, const char *key, lookangle &obj, lookangle *defval)
{
	return Serialize(arc, key, obj.value, defval ? &defval->value : nullptr);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class binangle
{
	uint32_t value;
	
	constexpr binangle(uint32_t v) : value(v) {}
	
	friend constexpr binangle bamang(uint32_t v);
	friend constexpr binangle q16ang(uint32_t v);
	friend constexpr binangle buildang(uint32_t v);
	friend binangle buildfang(double v);
	friend binangle radang(double v);
	friend binangle degang(double v);

	friend FSerializer &Serialize(FSerializer &arc, const char *key, binangle &obj, binangle *defval);
	
public:
	binangle() = default;
	binangle(const binangle &other) = default;
	// This class intentionally makes no allowances for implicit type conversions because those would render it ineffective.
	constexpr short asbuild() const { return value >> 21; }
	constexpr double asbuildf() const { return value * (1. / BAMUNIT); }
	constexpr fixed_t asq16() const { return value >> 5; }
	constexpr double asrad() const { return value * (pi::pi() / 0x80000000u); }
	constexpr double asdeg() const { return AngleToFloat(value); }
	constexpr uint32_t asbam() const { return value; }
	
	double fsin() const { return g_sin(asrad()); }
	double fcos() const { return g_cos(asrad()); }
	double ftan() const { return g_tan(asrad()); }
	int bsin(const int8_t& shift = 0) const { return ::bsin(asbuild(), shift); }
	int bcos(const int8_t& shift = 0) const { return ::bcos(asbuild(), shift); }

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

	constexpr binangle &operator+= (lookangle other)
	{
		value += other.value;
		return *this;
	}

	constexpr binangle &operator-= (lookangle other)
	{
		value -= other.value;
		return *this;
	}

	constexpr binangle operator+ (lookangle other) const
	{
		return binangle(value + other.value);
	}

	constexpr binangle operator- (lookangle other) const
	{
		return binangle(value - other.value);
	}

	constexpr binangle &operator<<= (const uint8_t shift)
	{
		value <<= shift;
		return *this;
	}

	constexpr binangle &operator>>= (const uint8_t shift)
	{
		value >>= shift;
		return *this;
	}

	constexpr binangle operator<< (const uint8_t shift) const
	{
		return binangle(value << shift);
	}

	constexpr binangle operator>> (const uint8_t shift) const
	{
		return binangle(value >> shift);
	}

};

inline constexpr binangle bamang(uint32_t v) { return binangle(v); }
inline constexpr binangle q16ang(uint32_t v) { return binangle(v << 5); }
inline constexpr binangle buildang(uint32_t v) { return binangle(v << BAMBITS); }
inline binangle buildfang(double v) { return binangle(xs_CRoundToUInt(v * BAMUNIT)); }
inline binangle radang(double v) { return binangle(xs_CRoundToUInt(v * (0x80000000u / pi::pi()))); }
inline binangle degang(double v) { return binangle(FloatToAngle(v)); }

inline FSerializer &Serialize(FSerializer &arc, const char *key, binangle &obj, binangle *defval)
{
	return Serialize(arc, key, obj.value, defval ? &defval->value : nullptr);
}


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
	friend constexpr fixedhoriz buildfhoriz(double v);
	friend fixedhoriz pitchhoriz(double v);
	friend fixedhoriz bamhoriz(int32_t v);

	friend FSerializer &Serialize(FSerializer &arc, const char *key, fixedhoriz &obj, fixedhoriz *defval);
	
public:
	fixedhoriz() = default;
	fixedhoriz(const fixedhoriz &other) = default;

	// This class intentionally makes no allowances for implicit type conversions because those would render it ineffective.
	constexpr short asbuild() const { return FixedToInt(value); }
	constexpr double asbuildf() const { return FixedToFloat(value); }
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

};

inline constexpr fixedhoriz q16horiz(fixed_t v) { return fixedhoriz(v); }
inline constexpr fixedhoriz buildhoriz(int v) { return fixedhoriz(IntToFixed(v)); }
inline constexpr fixedhoriz buildfhoriz(double v) { return fixedhoriz(FloatToFixed(v)); }
inline fixedhoriz pitchhoriz(double v) { return fixedhoriz(PitchToHoriz(v)); }
inline fixedhoriz bamhoriz(int32_t v) { return pitchhoriz(BAMToPitch(v)); }

inline FSerializer &Serialize(FSerializer &arc, const char *key, fixedhoriz &obj, fixedhoriz *defval)
{
	return Serialize(arc, key, obj.value, defval ? &defval->value : nullptr);
}


//---------------------------------------------------------------------------
//
// Double-precision implementation of `getangle()` with associated wrappers and helper functions.
//
//---------------------------------------------------------------------------

inline double bradarangf(const double& vect)
{
	return atan(vect) * BRadAngScale;
}
inline double bvectangf(const int32_t& x, const int32_t& y)
{
	if ((x | y) == 0)
	{
		return 0;
	}
	else if (x == 0)
	{
		return 512 + ((y < 0) << 10);
	}
	else if (y == 0)
	{
		return ((x < 0) << 10);
	}
	else if (x == y)
	{
		return 256 + ((x < 0) << 10);
	}
	else if (x == -y)
	{
		return 768 + ((x > 0) << 10);
	}
	else if (abs(x) > abs(y))
	{
		return fmod(bradarangf(double(y) / x) + ((x < 0) << 10), 2048.);
	}
	else
	{
		return fmod(bradarangf(double(x) / -y) + 512 + ((y < 0) << 10), 2048.);
	}
}
inline int32_t bvectang(const int32_t& x, const int32_t& y)
{
	return xs_CRoundToInt(bvectangf(x, y));
}
inline fixed_t bvectangq16(const int32_t& x, const int32_t& y)
{
	return FloatToFixed(bvectangf(x, y));
}
inline binangle bvectangbam(const int32_t& x, const int32_t& y)
{
	return bamang(xs_CRoundToUInt(bvectangf(x, y) * BAMUNIT));
}
