

template<typename TrackedType>
class TRACKER_NAME_
{
    public:
        TrackedType TrackedValue;

        inline TrackedType* operator & ()
        {
            TRACKER_GLOBAL_HOOK_((uintptr_t)&this->TrackedValue);
            return &this->TrackedValue;
        }

        inline TrackedType operator ++ ()
        {
            TRACKER_GLOBAL_HOOK_((uintptr_t)&this->TrackedValue);
            return ++this->TrackedValue;
        }

        inline TrackedType operator ++ (int)
        {
            TRACKER_GLOBAL_HOOK_((uintptr_t)&this->TrackedValue);
            return this->TrackedValue++;
        }

        inline TrackedType operator -- ()
        {
            TRACKER_GLOBAL_HOOK_((uintptr_t)&this->TrackedValue);
            return --this->TrackedValue;
        }

        inline TrackedType operator -- (int)
        {
            TRACKER_GLOBAL_HOOK_((uintptr_t)&this->TrackedValue);
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

#ifndef tracker_hpp_
#define tracker_hpp_

enum {
    TRACKER_NOOP_RIGHTHAND_EQUAL_ = 0,
    TRACKER_NOOP_RIGHTHAND_ZERO_,
    TRACKER_NOOP_RIGHTHAND_ONE_,
    __TRACKER_NEVER,
};

#endif

#define __TRACKER_RIGHTHAND_TYPE TrackedType rightHand
#define __TRACKER_RIGHTHAND rightHand
#include "tracker_operators.hpp"
#undef __TRACKER_RIGHTHAND_TYPE
#undef __TRACKER_RIGHTHAND

template<typename TrackedType>
inline TRACKER_NAME_<TrackedType>::operator TrackedType() const
{
    return this->TrackedValue;
}

template<typename TrackedType>
inline TrackedType TRACKER_NAME_<TrackedType>::cast() const
{
    return this->TrackedValue;
}
