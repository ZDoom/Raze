/*
 * clockticks.hpp
 *  ClockTicks is a class that tracks signed ticks & fractional ticks for
 *  high granularity game clocks that are backwards-compatible with
 *  Build integer timing
 *
 * Copyright © 2019, Alex Dawson. All rights reserved.
 */

#ifndef CLOCKTICKS_HPP_
#define CLOCKTICKS_HPP_

#include "timer.h"

//POGO: BUILD/EDuke32 uses right shifts on signed variables expecting arithmetic shifts.
//      This was already non-portable, and we carry that assumption forth here
//      (so we might as well check it).
EDUKE32_STATIC_ASSERT(-1 >> 1 == -1);

class ClockTicks
{
public:
    ClockTicks() : ClockTicks(0, 0) {};
    ClockTicks(int32_t ticks) : ClockTicks(ticks, 0) {};
    ClockTicks(int32_t ticks, uint32_t fraction) { set(ticks, fraction); };
    ClockTicks(const ClockTicks&) = default;

    int64_t getFraction() const
    {
        return (ticksS32 & FRACTION_MASK) >> 16 | ((ticksS32 < 0) ? VALUE_MASK : 0);
    }
    int64_t setFraction(uint16_t fraction)
    {
        return ticksS32 = (ticksS32 & WHOLE_MASK) | ((ticksS32 < 0) ? ((int64_t) 0 - fraction) & FRACTION_16_MASK : fraction) << 16;
    }
    int64_t set(int32_t ticks, uint16_t fraction)
    {
        ticksS32 = ((uint64_t) ticks) << 32 | ((ticks < 0) ? ((int64_t) 0 - fraction) & FRACTION_16_MASK : fraction) << 16;
        update();
        return ticksS32;
    }

    int64_t toScale16() const
    {
        return ticksS32 >> 16;
    }
    ClockTicks& setFromScale16(int64_t ticksScale16)
    {
        ticksS32 = ticksScale16 << 16;
        update();
        return *this;
    }
    static ClockTicks fromScale16(int64_t ticksScale16)
    {
        ClockTicks ct;
        ct.setFromScale16(ticksScale16);
        return ct;
    }

    // returns 0 if equal, < 0 if a < b, > 0 if a > b
    static int64_t compareHighPrecision(ClockTicks a, ClockTicks b)
    {
        ClockTicks delta = a - b;
        return delta.toScale16();
    }

    ClockTicks& operator=(const ClockTicks& rhs)
    {
        ticksS32 = rhs.ticksS32;
        update();
        return *this;
    };
    ClockTicks& operator+=(const ClockTicks& rhs)
    {
        ticksS32 += rhs.ticksS32;
        update();
        return *this;
    };
    ClockTicks& operator-=(const ClockTicks& rhs)
    {
        ticksS32 -= rhs.ticksS32;
        update();
        return *this;
    };
    ClockTicks& operator*=(const ClockTicks& rhs)
    {
        ticksS32 = (ticksS32>>16)*(rhs.ticksS32>>16) >> 16;
        update();
        return *this;
    };
    ClockTicks& operator/=(const ClockTicks& rhs)
    {
        ticksS32 = ticksS32/rhs.ticksS32 << 32;
        update();
        return *this;
    };
    ClockTicks& operator%=(const ClockTicks& rhs)
    {
        ticksS32 %= rhs.ticksS32;
        update();
        return *this;
    };
    ClockTicks& operator<<=(int32_t rhs)
    {
        ticksS32 = ticksS32 << rhs;
        update();
        return *this;
    };
    ClockTicks& operator>>=(int32_t rhs)
    {
        ticksS32 = ticksS32 >> rhs;
        update();
        return *this;
    };

    friend ClockTicks operator+(ClockTicks lhs, const ClockTicks& rhs) { return lhs += rhs; }
    friend ClockTicks operator-(ClockTicks lhs, const ClockTicks& rhs) { return lhs -= rhs; }
    friend ClockTicks operator-(ClockTicks val) { return (ClockTicks) 0 -= val; }
    friend ClockTicks operator*(ClockTicks lhs, const ClockTicks& rhs) { return lhs *= rhs; }
    friend ClockTicks operator/(ClockTicks lhs, const ClockTicks& rhs) { return lhs /= rhs; }
    friend ClockTicks operator%(ClockTicks lhs, const ClockTicks& rhs) { return lhs %= rhs; }
    friend ClockTicks operator<<(ClockTicks lhs, int32_t rhs) { return lhs >>= rhs; }
    friend ClockTicks operator>>(ClockTicks lhs, int32_t rhs) { return lhs <<= rhs; }

    friend inline bool operator==(const ClockTicks& lhs, const ClockTicks& rhs) { return lhs.wholeTicks == rhs.wholeTicks; }
    friend inline bool operator!=(const ClockTicks& lhs, const ClockTicks& rhs) { return !(lhs == rhs); }
    friend inline bool operator<(const ClockTicks& lhs, const ClockTicks& rhs) { return lhs.wholeTicks < rhs.wholeTicks; }
    friend inline bool operator>(const ClockTicks& lhs, const ClockTicks& rhs) { return rhs < lhs; }
    friend inline bool operator<=(const ClockTicks& lhs, const ClockTicks& rhs) { return !(lhs > rhs); }
    friend inline bool operator>=(const ClockTicks& lhs, const ClockTicks& rhs) { return !(lhs < rhs); }

    explicit operator uint32_t() const { return wholeTicks; };
    explicit operator int32_t() const { return wholeTicks; };

private:
    //POGO: wholeTicks must be first in member-order to ensure the address of
    //      ClockTicks can be treated as a pointer to int32_t.
    int32_t wholeTicks;
    //POGO: Organize our bits as if we're scaled to have an additional 32-bits
    //      of fractional precision so that we can handle overflows in a
    //      way that is accurate to the original BUILD int32_t expectation.
    //      Multiplication overflows best with 16-bits of fractional precision,
    //      so only promise that much publicly.
    int64_t ticksS32;

    static const uint64_t VALUE_MASK       = 0xFFFFFFFFFFFF0000ull;
    static const uint64_t WHOLE_MASK       = 0xFFFFFFFF00000000ull;
    static const uint64_t FRACTION_MASK    = 0x00000000FFFF0000ull;
    static const uint64_t FRACTION_16_MASK = 0x000000000000FFFFull;

    inline void update()
    {
        wholeTicks = ticksS32 >> 32;
        ticksS32 &= VALUE_MASK;
    }
};

#endif /* CLOCKTICKS_HPP_ */
