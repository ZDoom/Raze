

template<typename TrackedType>
class __TRACKER_NAME
{
        TrackedType TrackedValue;

    public:
        TrackedType operator = (TrackedType);
        TrackedType operator = (__TRACKER_NAME<TrackedType>);
        TrackedType operator += (TrackedType);
        TrackedType operator += (__TRACKER_NAME<TrackedType>);

        operator TrackedType();
};

enum {
    __TRACKER_NOOP_RIGHTHAND_EQUAL = 0,
    __TRACKER_NOOP_RIGHTHAND_ZERO,
    __TRACKER_NEVER,
};

#define __TRACKER_RIGHTHAND_TYPE __TRACKER_NAME<TrackedType> rightHand
#define __TRACKER_RIGHTHAND rightHand.TrackedValue
#include "tracker_operators.hpp"
#undef __TRACKER_RIGHTHAND_TYPE
#undef __TRACKER_RIGHTHAND

#define __TRACKER_RIGHTHAND_TYPE TrackedType rightHand
#define __TRACKER_RIGHTHAND rightHand
#include "tracker_operators.hpp"
#undef __TRACKER_RIGHTHAND_TYPE
#undef __TRACKER_RIGHTHAND

template<typename TrackedType>
__TRACKER_NAME<TrackedType>::operator TrackedType()
{
    return this->TrackedValue;
}
