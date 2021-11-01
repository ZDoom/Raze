// ====================================================================================================================
// ====================================================================================================================
//  xs_Float.h
//
// Source: "Know Your FPU: Fixing Floating Fast"
//         http://www.stereopsis.com/sree/fpu2006.html
//
// xs_CRoundToInt:  Round toward nearest, but ties round toward even (just like FISTP)
// xs_ToInt:        Round toward zero, just like the C (int) cast
// xs_FloorToInt:   Round down
// xs_CeilToInt:    Round up
// xs_RoundToInt:   Round toward nearest, but ties round up
// ====================================================================================================================
// ====================================================================================================================
#ifndef _xs_FLOAT_H_
#define _xs_FLOAT_H_

#include <stdint.h>

// ====================================================================================================================
//  Defines
// ====================================================================================================================
#ifndef _xs_DEFAULT_CONVERSION
#define _xs_DEFAULT_CONVERSION      0
#endif //_xs_DEFAULT_CONVERSION


#if __BIG_ENDIAN__
	#define _xs_iexp_				0
	#define _xs_iman_				1
#else
	#define _xs_iexp_				1       //intel is little endian
	#define _xs_iman_				0
#endif //BigEndian_

#ifdef __GNUC__
#define finline inline
#else
#define finline __forceinline
#endif

typedef double					real64;


struct _xs_doubleints
{
	union
	{
		real64 val;
		uint32_t ival[2];
	};

	constexpr _xs_doubleints(real64 v) : val(v) {}
};

#if 0
#define _xs_doublecopysgn(a,b)      ((int32_t*)&a)[_xs_iexp_]&=~(((int32_t*)&b)[_xs_iexp_]&0x80000000)
#define _xs_doubleisnegative(a)     ((((int32_t*)&a)[_xs_iexp_])|0x80000000)
#endif

// ====================================================================================================================
//  Constants
// ====================================================================================================================
constexpr real64 _xs_doublemagic            = real64 (6755399441055744.0);      //2^52 * 1.5,  uses limited precisicion to floor
constexpr real64 _xs_doublemagicdelta       = (1.5e-8);                         //almost .5f = .5f + 1e^(number of exp bit)
constexpr real64 _xs_doublemagicroundeps    = (.5f-_xs_doublemagicdelta);       //almost .5f = .5f - 1e^(number of exp bit)


// ====================================================================================================================
// ====================================================================================================================
//  Inline implementation
// ====================================================================================================================
// ====================================================================================================================
finline constexpr int32_t xs_CRoundToInt(real64 val, real64 dmr = _xs_doublemagic)
{
#if _xs_DEFAULT_CONVERSION==0
	return _xs_doubleints(val + dmr).ival[_xs_iman_];
#else
    return int32_t(floor(val+.5));
#endif
}


// ====================================================================================================================
finline constexpr int32_t xs_ToInt(real64 val, real64 dme = -_xs_doublemagicroundeps)
{
    /* unused - something else I tried...
            _xs_doublecopysgn(dme,val);
            return xs_CRoundToInt(val+dme);
            return 0;
    */

#if _MSC_VER >= 1400
	// VC++ 2005's standard cast is a little bit faster than this
	// magic number code. (Which is pretty amazing!) SSE has the
	// fastest C-style float->int conversion, but unfortunately,
	// checking for SSE support every time you need to do a
	// conversion completely negates its performance advantage.
	return int32_t(val);
#else
#if _xs_DEFAULT_CONVERSION==0
	return (val<0) ?   xs_CRoundToInt(val-dme) : 
					   xs_CRoundToInt(val+dme);
#else
    return int32_t(val);
#endif
#endif
}


// ====================================================================================================================
finline constexpr int32_t xs_FloorToInt(real64 val, real64 dme = _xs_doublemagicroundeps)
{
#if _xs_DEFAULT_CONVERSION==0
    return xs_CRoundToInt (val - dme);
#else
    return floor(val);
#endif
}


// ====================================================================================================================
finline constexpr int32_t xs_CeilToInt(real64 val, real64 dme = _xs_doublemagicroundeps)
{
#if _xs_DEFAULT_CONVERSION==0
    return xs_CRoundToInt (val + dme);
#else
    return ceil(val);
#endif
}


// ====================================================================================================================
finline constexpr int32_t xs_RoundToInt(real64 val)
{
#if _xs_DEFAULT_CONVERSION==0
	// Yes, it is important that two fadds be generated, so you cannot override the dmr
	// passed to xs_CRoundToInt with _xs_doublemagic + _xs_doublemagicdelta. If you do,
	// you'll end up with Banker's Rounding again.
    return xs_CRoundToInt (val + _xs_doublemagicdelta);
#else
    return floor(val+.5);
#endif
}


// ====================================================================================================================
finline constexpr int32_t xs_ToFixed(int32_t n, real64 val)
{
    #if _xs_DEFAULT_CONVERSION==0
        return xs_CRoundToInt(val, _xs_doublemagic/(1<<n));
    #else
        return (long)((val)*(1<<N));
    #endif
}


//int32_t versions
finline constexpr int32_t xs_CRoundToInt      (int32_t val)   {return val;}
finline constexpr int32_t xs_ToInt            (int32_t val)   {return val;}


// ====================================================================================================================
// ====================================================================================================================
//  Unsigned variants
// ====================================================================================================================
// ====================================================================================================================
finline constexpr uint32_t xs_CRoundToUInt(real64 val)
{
	return (uint32_t)xs_CRoundToInt(val);
}

finline constexpr uint32_t xs_FloorToUInt(real64 val)
{
	return (uint32_t)xs_FloorToInt(val);
}

finline constexpr uint32_t xs_CeilToUInt(real64 val)
{
	return (uint32_t)xs_CeilToInt(val);
}

finline constexpr uint32_t xs_RoundToUInt(real64 val)
{
	return (uint32_t)xs_RoundToInt(val);
}


// ====================================================================================================================
//  Fix Class
// ====================================================================================================================
template <int32_t N> class xs_Fix
{
public:
    typedef int32_t Fix;

    // ====================================================================================================================
    //  Basic Conversion from Numbers
    // ====================================================================================================================
    finline static constexpr Fix       ToFix       (int32_t val)  {return val<<N;}
    finline static constexpr Fix       ToFix       (real64 val)   {return xs_ConvertToFixed(val);}

    // ====================================================================================================================
    //  Basic Conversion to Numbers
    // ====================================================================================================================
    finline static constexpr real64    ToReal      (Fix f)        {return real64(f)/real64(1<<N);}
    finline static constexpr int32_t     ToInt     (Fix f)        {return f>>N;}



protected:
    // ====================================================================================================================
    // Helper function - mainly to preserve _xs_DEFAULT_CONVERSION
    // ====================================================================================================================
    finline static constexpr int32_t xs_ConvertToFixed (real64 val)
    {
    #if _xs_DEFAULT_CONVERSION==0
        return xs_CRoundToInt(val, _xs_doublemagic/(1<<N));
    #else
        return (long)((val)*(1<<N));
    #endif
    }
};


// ====================================================================================================================
// ====================================================================================================================
#endif // _xs_FLOAT_H_