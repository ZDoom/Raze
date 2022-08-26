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
#include "basics.h"
#include "m_fixed.h"
#include "vectors.h"
#include "xs_Float.h"	// needed for reliably overflowing float->int conversions.
#include "serializer.h"
#include "math/cmath.h"

class FSerializer;

enum
{
	BAMBITS = 21,
	BAMUNIT = 1 << BAMBITS,
	SINTABLEBITS = 30,
	SINTABLEUNIT = 1 << SINTABLEBITS,
	BUILDSINBITS = 14,
	BUILDSINSHIFT = SINTABLEBITS - BUILDSINBITS,
};

//---------------------------------------------------------------------------
//
// Constants used for Build sine/cosine functions.
//
//---------------------------------------------------------------------------

constexpr double BAngRadian = pi::pi() * (1. / 1024.);
constexpr double BAngToDegree = 360. / 2048.;

extern int sintable[2048];

inline constexpr double sinscale(const int shift)
{
	return shift >= -BUILDSINBITS ? uint64_t(1) << (BUILDSINBITS + shift) : 1. / (uint64_t(1) << abs(BUILDSINBITS + shift));
}

//---------------------------------------------------------------------------
//
// Build sine inline functions.
//
//---------------------------------------------------------------------------

inline int bsin(const int ang, int shift = 0)
{
	return (shift -= BUILDSINSHIFT) < 0 ? sintable[ang & 2047] >> abs(shift) : sintable[ang & 2047] << shift;
}
inline double bsinf(const double ang, const int shift = 0)
{
	return g_sinbam(ang * BAMUNIT) * sinscale(shift);
}


//---------------------------------------------------------------------------
//
// Build cosine inline functions.
//
//---------------------------------------------------------------------------

inline int bcos(const int ang, int shift = 0)
{
	return (shift -= BUILDSINSHIFT) < 0 ? sintable[(ang + 512) & 2047] >> abs(shift) : sintable[(ang + 512) & 2047] << shift;
}
inline double bcosf(const double ang, const int shift = 0)
{
	return g_cosbam(ang * BAMUNIT) * sinscale(shift);
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

	constexpr int32_t tosigned() const { return int32_t(value); }

public:
	binangle() = default;
	binangle(const binangle &other) = default;
	binangle& operator=(const binangle& other) = default;
	// This class intentionally makes no allowances for implicit type conversions because those would render it ineffective.
	constexpr short asbuild() const { return value >> BAMBITS; }
	constexpr double asbuildf() const { return value * (1. / +BAMUNIT); }
	constexpr fixed_t asq16() const { return value >> 5; }
	constexpr uint32_t asbam() const { return value; }
	constexpr double asrad() const { return value * (pi::pi() / 0x80000000u); }
	constexpr double asdeg() const { return DAngle::fromBam(value).Degrees(); }
	constexpr short signedbuild() const { return tosigned() >> BAMBITS; }
	constexpr double signedbuildf() const { return tosigned() * (1. / +BAMUNIT); }
	constexpr fixed_t signedq16() const { return tosigned() >> 5; }
	constexpr int32_t signedbam() const { return tosigned(); }
	constexpr double signedrad() const { return tosigned() * (pi::pi() / 0x80000000u); }
	constexpr double signeddeg() const { return DAngle::fromBam(tosigned()).Degrees(); }

	double fsin() const { return g_sinbam(asbam()); }
	double fcos() const { return g_cosbam(asbam()); }
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

	constexpr binangle operator- () const
	{
		return binangle(0 - value);
	}

	constexpr binangle &operator<<= (const uint8_t shift)
	{
		value = tosigned() << shift;
		return *this;
	}

	constexpr binangle &operator>>= (const uint8_t shift)
	{
		value = tosigned() >> shift;
		return *this;
	}

	constexpr binangle operator<< (const uint8_t shift) const
	{
		return binangle(tosigned() << shift);
	}

	constexpr binangle operator>> (const uint8_t shift) const
	{
		return binangle(tosigned() >> shift);
	}
};

inline constexpr binangle bamang(uint32_t v) { return binangle(v); }
inline constexpr binangle q16ang(uint32_t v) { return binangle(v << 5); }
inline constexpr binangle buildang(uint32_t v) { return binangle(v << BAMBITS); }
inline binangle buildfang(double v) { return binangle(xs_ToFixed(BAMBITS, v)); }
inline binangle radang(double v) { return binangle(xs_CRoundToUInt(v * (0x80000000u / pi::pi()))); }
inline binangle degang(double v) { return binangle(FloatToAngle(v)); }

inline FSerializer &Serialize(FSerializer &arc, const char *key, binangle &obj, binangle *defval)
{
	return Serialize(arc, key, obj.value, defval ? &defval->value : nullptr);
}


//---------------------------------------------------------------------------
//
// Functions for use with fixedhoriz and friendly functions.
//
//---------------------------------------------------------------------------

inline double HorizToPitch(double horiz) { return atan2(horiz, 128) * (180. / pi::pi()); }
inline double HorizToPitch(fixed_t q16horiz) { return atan2(q16horiz, IntToFixed(128)) * (180. / pi::pi()); }
inline fixed_t PitchToHoriz(double pitch) { return xs_CRoundToInt(clamp<double>(IntToFixed(128) * tan(pitch * (pi::pi() / 180.)), INT32_MIN, INT32_MAX)); }
inline int32_t PitchToBAM(double pitch) { return xs_CRoundToInt(clamp<double>(pitch * (0x80000000u / 90.), INT32_MIN, INT32_MAX)); }
inline constexpr double BAMToPitch(int32_t bam) { return bam * (90. / 0x80000000u); }


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
	friend fixedhoriz buildfhoriz(double v);
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
inline fixedhoriz buildfhoriz(double v) { return fixedhoriz(FloatToFixed(v)); }
inline fixedhoriz pitchhoriz(double v) { return fixedhoriz(PitchToHoriz(v)); }
inline fixedhoriz bamhoriz(int32_t v) { return pitchhoriz(BAMToPitch(v)); }

inline FSerializer &Serialize(FSerializer &arc, const char *key, fixedhoriz &obj, fixedhoriz *defval)
{
	return Serialize(arc, key, obj.value, defval ? &defval->value : nullptr);
}


//---------------------------------------------------------------------------
//
// High precision vector angle function, mainly for the renderer.
//
//---------------------------------------------------------------------------

inline binangle bvectangbam(double x, double y)
{
	return radang(atan2(y, x));
}


//---------------------------------------------------------------------------
//
// Interpolation functions for use throughout games.
//
//---------------------------------------------------------------------------

inline constexpr int32_t interpolatedvalue(int32_t oval, int32_t val, double const smoothratio, int const scale = 16)
{
	return oval + MulScale(val - oval, int(smoothratio), scale);
}

inline constexpr int32_t interpolatedvalue(int32_t oval, int32_t val, int const smoothratio, int const scale = 16)
{
	return oval + MulScale(val - oval, smoothratio, scale);
}

inline constexpr double interpolatedvaluef(double oval, double val, double const smoothratio, int const scale = 16)
{
	return oval + MulScaleF(val - oval, smoothratio, scale);
}

inline constexpr int32_t interpolatedangle(int32_t oang, int32_t ang, double const smoothratio, int const scale = 16)
{
	return oang + MulScale(((ang + 1024 - oang) & 2047) - 1024, int(smoothratio), scale);
}

inline constexpr int32_t interpolatedangle(int32_t oang, int32_t ang, int const smoothratio, int const scale = 16)
{
	return oang + MulScale(((ang + 1024 - oang) & 2047) - 1024, smoothratio, scale);
}

inline constexpr binangle interpolatedangle(binangle oang, binangle ang, double const smoothratio, int const scale = 16)
{
	return bamang(oang.asbam() + MulScale(((ang.asbam() + 0x80000000 - oang.asbam()) & 0xFFFFFFFF) - 0x80000000, int(smoothratio), scale));
}

inline constexpr binangle interpolatedangle(binangle oang, binangle ang, int const smoothratio, int const scale = 16)
{
	return bamang(oang.asbam() + MulScale(((ang.asbam() + 0x80000000 - oang.asbam()) & 0xFFFFFFFF) - 0x80000000, smoothratio, scale));
}

inline constexpr fixedhoriz interpolatedhorizon(fixedhoriz oval, fixedhoriz val, double const smoothratio, int const scale = 16)
{
	return q16horiz(oval.asq16() + MulScale((val - oval).asq16(), int(smoothratio), scale));
}

inline constexpr fixedhoriz interpolatedhorizon(fixedhoriz oval, fixedhoriz val, int const smoothratio, int const scale = 16)
{
	return q16horiz(oval.asq16() + MulScale((val - oval).asq16(), smoothratio, scale));
}
