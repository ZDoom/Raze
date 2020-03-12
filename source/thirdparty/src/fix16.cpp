#include "fix16.h"
#include "fix16_int64.h"

/* Subtraction and addition with overflow detection.
 * The versions without overflow detection are inlined in the header.
 */
#ifndef FIXMATH_NO_OVERFLOW
fix16_t fix16_add(fix16_t a, fix16_t b)
{
    // Use unsigned integers because overflow with signed integers is
    // an undefined operation (http://www.airs.com/blog/archives/120).
    uint32_t _a = a, _b = b;
    uint32_t sum = _a + _b;

    // Overflow can only happen if sign of a == sign of b, and then
    // it causes sign of sum != sign of a.
    if (!((_a ^ _b) & 0x80000000) && ((_a ^ sum) & 0x80000000))
        return FIX16_OVERFLOW;

    return sum;
}

fix16_t fix16_sub(fix16_t a, fix16_t b)
{
    uint32_t _a = a, _b = b;
    uint32_t diff = _a - _b;

    // Overflow can only happen if sign of a != sign of b, and then
    // it causes sign of diff != sign of a.
    if (((_a ^ _b) & 0x80000000) && ((_a ^ diff) & 0x80000000))
        return FIX16_OVERFLOW;

    return diff;
}

/* Saturating arithmetic */
fix16_t fix16_sadd(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_add(a, b);

    if (result == FIX16_OVERFLOW)
        return (a >= 0) ? FIX16_MAX : FIX16_MIN;

    return result;
}

fix16_t fix16_ssub(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_sub(a, b);

    if (result == FIX16_OVERFLOW)
        return (a >= 0) ? FIX16_MAX : FIX16_MIN;

    return result;
}
#endif



/* 64-bit implementation for fix16_mul. Fastest version for e.g. ARM Cortex M3.
 * Performs a 32*32 -> 64bit multiplication. The middle 32 bits are the result,
 * bottom 16 bits are used for rounding, and upper 16 bits are used for overflow
 * detection.
 */

fix16_t fix16_mul(fix16_t inArg0, fix16_t inArg1)
{
    int64_t product = (int64_t)inArg0 * inArg1;

    #ifndef FIXMATH_NO_OVERFLOW
    // The upper 17 bits should all be the same (the sign).
    uint32_t upper = (product >> 47);
    #endif

    if (product < 0)
    {
        #ifndef FIXMATH_NO_OVERFLOW
        if (~upper)
                return FIX16_OVERFLOW;
        #endif

        #ifndef FIXMATH_NO_ROUNDING
        // This adjustment is required in order to round -1/2 correctly
        product--;
        #endif
    }
    else
    {
        #ifndef FIXMATH_NO_OVERFLOW
        if (upper)
                return FIX16_OVERFLOW;
        #endif
    }

    #ifdef FIXMATH_NO_ROUNDING
    return product >> 16;
    #else
    fix16_t result = product >> 16;
    result += (product & 0x8000) >> 15;

    return result;
    #endif
}

#ifndef FIXMATH_NO_OVERFLOW
/* Wrapper around fix16_mul to add saturating arithmetic. */
fix16_t fix16_smul(fix16_t inArg0, fix16_t inArg1)
{
    fix16_t result = fix16_mul(inArg0, inArg1);

    if (result == FIX16_OVERFLOW)
    {
        if ((inArg0 >= 0) == (inArg1 >= 0))
            return FIX16_MAX;
        else
            return FIX16_MIN;
    }

    return result;
}
#endif

/* 32-bit implementation of fix16_div. Fastest version for e.g. ARM Cortex M3.
 * Performs 32-bit divisions repeatedly to reduce the remainder. For this to
 * be efficient, the processor has to have 32-bit hardware division.
 */
#ifdef __GNUC__
// Count leading zeros, using processor-specific instruction if available.
#define clz(x) (__builtin_clzl(x) - (8 * sizeof(long) - 32))
#else
static uint8_t clz(uint32_t x)
{
    uint8_t result = 0;
    if (x == 0) return 32;
    while (!(x & 0xF0000000)) { result += 4; x <<= 4; }
    while (!(x & 0x80000000)) { result += 1; x <<= 1; }
    return result;
}
#endif

fix16_t fix16_div(fix16_t a, fix16_t b)
{
    // This uses a hardware 32/32 bit division multiple times, until we have
    // computed all the bits in (a<<17)/b. Usually this takes 1-3 iterations.

    if (b == 0)
            return FIX16_MIN;

    uint32_t remainder = (a >= 0) ? a : (-a);
    uint32_t divider = (b >= 0) ? b : (-b);
    uint32_t quotient = 0;
    int bit_pos = 17;

    // Kick-start the division a bit.
    // This improves speed in the worst-case scenarios where N and D are large
    // It gets a lower estimate for the result by N/(D >> 17 + 1).
    if (divider & 0xFFF00000)
    {
        uint32_t shifted_div = ((divider >> 17) + 1);
        quotient = remainder / shifted_div;
        remainder -= ((uint64_t)quotient * divider) >> 17;
    }

    // If the divider is divisible by 2^n, take advantage of it.
    while (!(divider & 0xF) && bit_pos >= 4)
    {
        divider >>= 4;
        bit_pos -= 4;
    }

    while (remainder && bit_pos >= 0)
    {
        // Shift remainder as much as we can without overflowing
        int shift = clz(remainder);
        if (shift > bit_pos) shift = bit_pos;
        remainder <<= shift;
        bit_pos -= shift;

        uint32_t div = remainder / divider;
        remainder = remainder % divider;
        quotient += div << bit_pos;

        #ifndef FIXMATH_NO_OVERFLOW
        if (div & ~(0xFFFFFFFF >> bit_pos))
                return FIX16_OVERFLOW;
        #endif

        remainder <<= 1;
        bit_pos--;
    }

    #ifndef FIXMATH_NO_ROUNDING
    // Quotient is always positive so rounding is easy
    quotient++;
    #endif

    fix16_t result = quotient >> 1;

    // Figure out the sign of the result
    if ((a ^ b) & 0x80000000)
    {
        #ifndef FIXMATH_NO_OVERFLOW
        if (result == FIX16_MIN)
                return FIX16_OVERFLOW;
        #endif

        result = -result;
    }

    return result;
}

#ifndef FIXMATH_NO_OVERFLOW
/* Wrapper around fix16_div to add saturating arithmetic. */
fix16_t fix16_sdiv(fix16_t inArg0, fix16_t inArg1)
{
    fix16_t result = fix16_div(inArg0, inArg1);

    if (result == FIX16_OVERFLOW)
    {
        if ((inArg0 >= 0) == (inArg1 >= 0))
            return FIX16_MAX;
        else
            return FIX16_MIN;
    }

    return result;
}
#endif

fix16_t fix16_lerp8(fix16_t inArg0, fix16_t inArg1, uint8_t inFract)
{
    int64_t tempOut = int64_mul_i32_i32(inArg0, ((1 << 8) - inFract));
    tempOut = int64_add(tempOut, int64_mul_i32_i32(inArg1, inFract));
    tempOut = int64_shift(tempOut, -8);
    return (fix16_t)int64_lo(tempOut);
}

fix16_t fix16_lerp16(fix16_t inArg0, fix16_t inArg1, uint16_t inFract)
{
    int64_t tempOut = int64_mul_i32_i32(inArg0, (((int32_t)1 << 16) - inFract));
    tempOut = int64_add(tempOut, int64_mul_i32_i32(inArg1, inFract));
    tempOut = int64_shift(tempOut, -16);
    return (fix16_t)int64_lo(tempOut);
}

fix16_t fix16_lerp32(fix16_t inArg0, fix16_t inArg1, uint32_t inFract)
{
    int64_t tempOut;
    tempOut  = ((int64_t)inArg0 * (0 - inFract));
    tempOut	+= ((int64_t)inArg1 * inFract);
    tempOut >>= 32;
    return (fix16_t)tempOut;
}

static const uint32_t scales[8] = {
    /* 5 decimals is enough for full fix16_t precision */
    1, 10, 100, 1000, 10000, 100000, 100000, 100000
};

static char *itoa_loop(char *buf, uint32_t scale, uint32_t value, bool skip)
{
    while (scale)
    {
        unsigned digit = (value / scale);

        if (!skip || digit || scale == 1)
        {
            skip = false;
            *buf++ = '0' + digit;
            value %= scale;
        }

        scale /= 10;
    }
    return buf;
}

void fix16_to_str(fix16_t value, char *buf, int decimals)
{
    uint32_t uvalue = (value >= 0) ? value : -value;
    if (value < 0)
        *buf++ = '-';

    /* Separate the integer and decimal parts of the value */
    unsigned intpart = uvalue >> 16;
    uint32_t fracpart = uvalue & 0xFFFF;
    uint32_t scale = scales[decimals & 7];
    fracpart = fix16_mul(fracpart, scale);

    if (fracpart >= scale)
    {
        /* Handle carry from decimal part */
        intpart++;
        fracpart -= scale;
    }

    /* Format integer part */
    buf = itoa_loop(buf, 10000, intpart, true);

    /* Format decimal part (if any) */
    if (scale != 1)
    {
        *buf++ = '.';
        buf = itoa_loop(buf, scale / 10, fracpart, false);
    }

    *buf = '\0';
}

fix16_t fix16_from_str(const char *buf)
{
    while (isspace(*buf))
        buf++;

    /* Decode the sign */
    bool negative = (*buf == '-');
    if (*buf == '+' || *buf == '-')
        buf++;

    /* Decode the integer part */
    uint32_t intpart = 0;
    int count = 0;
    while (isdigit(*buf))
    {
        intpart *= 10;
        intpart += *buf++ - '0';
        count++;
    }

    if (count == 0 || count > 5
        || intpart > 32768 || (!negative && intpart > 32767))
        return FIX16_OVERFLOW;

    fix16_t value = intpart << 16;

    /* Decode the decimal part */
    if (*buf == '.' || *buf == ',')
    {
        buf++;

        uint32_t fracpart = 0;
        uint32_t scale = 1;
        while (isdigit(*buf) && scale < 100000)
        {
            scale *= 10;
            fracpart *= 10;
            fracpart += *buf++ - '0';
        }

        value += fix16_div(fracpart, scale);
    }

    /* Verify that there is no garbage left over */
    while (*buf != '\0')
    {
        if (!isdigit(*buf) && !isspace(*buf))
            return FIX16_OVERFLOW;

        buf++;
    }

    return negative ? -value : value;
}
