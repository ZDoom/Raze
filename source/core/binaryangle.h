#pragma once

#include <math.h>
#include "m_fixed.h"
#include "xs_Float.h"	// needed for reliably overflowing float->int conversions.
#include "build.h"

// type safe representations of high precision angle and horizon values. Angle uses natural 32 bit overflow to clamp to one rotation.

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
	
	constexpr binangle operator- () const
	{
		return binangle(-value);
	}

	constexpr binangle operator+ (binangle other) const
	{
		return binangle(value + other.value);
	}

	constexpr binangle operator- (binangle other) const
	{
		return binangle(value - other.value);
	}
	
	void interpolate(binangle a1, binangle a2, fix16_t smoothratio)
	{
		// Calculate in floating point to reduce the error caused by overflows which are to be expected here and then downconvert using a method that is safe to overflow.
		// We do not want fixed point multiplications here to trash the result.
		double smooth = smoothratio / 65536.f;
		value = xs_CRoundToUInt(double(a1.asbam()) + smooth * (double(a2.asbam()) - double(a1.asbam())));
	}


};

class fixedhoriz
{
	unsigned int value;
	
	constexpr fixedhoriz(unsigned int v) : value(v) {}
	
	friend constexpr fixedhoriz q16horiz(int v);
	friend constexpr fixedhoriz buildhoriz(int v);
	
public:
	fixedhoriz() = default;
	fixedhoriz(const fixedhoriz &other) = default;

	// This class intentionally makes no allowances for implicit type conversions because those would render it ineffective.
	short asbuild() const { return value >> 16; }
	constexpr fixed_t asq16() const { return value; }
	
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

inline constexpr fixedhoriz q16horiz(int v) { return fixedhoriz(v); }
inline constexpr fixedhoriz buildhoriz(int v) { return fixedhoriz(v << 16); }

