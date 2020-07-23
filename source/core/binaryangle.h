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
	

};


inline constexpr binangle bamang(unsigned int v) { return binangle(v); }
inline constexpr binangle q16ang(unsigned int v) { return binangle(v << 5); }
inline constexpr binangle buildang(unsigned int v) { return binangle(v << 21); }
inline binangle radang(double v) { return binangle(xs_CRoundToUInt(v * (0x80000000u / binangle::pi()))); }
inline binangle degang(double v) { return binangle(xs_CRoundToUInt(v * (0x40000000 / 90.))); }

