

template<typename TrackedType>
class __TRACKER_NAME
{
    public:
        TrackedType TrackedValue;

        inline TrackedType* operator & ()
        {
            __TRACKER_GLOBAL_HOOK((uintptr_t)&this->TrackedValue);
            return &this->TrackedValue;
        }

        inline TrackedType operator ++ ()
        {
            __TRACKER_GLOBAL_HOOK((uintptr_t)&this->TrackedValue);
            return ++this->TrackedValue;
        }

        inline TrackedType operator ++ (int)
        {
            __TRACKER_GLOBAL_HOOK((uintptr_t)&this->TrackedValue);
            return this->TrackedValue++;
        }

        inline TrackedType operator -- ()
        {
            __TRACKER_GLOBAL_HOOK((uintptr_t)&this->TrackedValue);
            return --this->TrackedValue;
        }

        inline TrackedType operator -- (int)
        {
            __TRACKER_GLOBAL_HOOK((uintptr_t)&this->TrackedValue);
            return this->TrackedValue--;
        }

        inline TrackedType operator = (TrackedType);

        inline TrackedType operator += (TrackedType);

        inline TrackedType operator -= (TrackedType);

        inline TrackedType operator *= (TrackedType);

        inline TrackedType operator /= (TrackedType);

        inline TrackedType operator |= (TrackedType);

        inline TrackedType operator &= (TrackedType);

        inline TrackedType operator ^= (TrackedType);

        inline TrackedType operator <<= (TrackedType);

        inline TrackedType operator >>= (TrackedType);

        inline operator TrackedType() const;

        inline TrackedType cast() const;
};

#ifndef __tracker_hpp
#define __tracker_hpp

enum {
    __TRACKER_NOOP_RIGHTHAND_EQUAL = 0,
    __TRACKER_NOOP_RIGHTHAND_ZERO,
    __TRACKER_NOOP_RIGHTHAND_ONE,
    __TRACKER_NEVER,
};

#endif

#define __TRACKER_RIGHTHAND_TYPE TrackedType rightHand
#define __TRACKER_RIGHTHAND rightHand
#include "tracker_operators.hpp"
#undef __TRACKER_RIGHTHAND_TYPE
#undef __TRACKER_RIGHTHAND

template<typename TrackedType>
__TRACKER_NAME<TrackedType>::operator TrackedType() const
{
    return this->TrackedValue;
}

template<typename TrackedType>
inline TrackedType __TRACKER_NAME<TrackedType>::cast() const
{
    return this->TrackedValue;
}
