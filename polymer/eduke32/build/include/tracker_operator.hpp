
template<typename TrackedType>
inline TrackedType TRACKER_NAME_<TrackedType>::operator TRACKER_OPERATOR_ (__TRACKER_RIGHTHAND_TYPE)
{
    bool isNoop;

    switch (TRACKER_NOOP_) {
        case TRACKER_NOOP_RIGHTHAND_EQUAL_:
            isNoop = this->TrackedValue == __TRACKER_RIGHTHAND;
            break;
        case TRACKER_NOOP_RIGHTHAND_ZERO_:
            isNoop = __TRACKER_RIGHTHAND == 0;
            break;
        case TRACKER_NOOP_RIGHTHAND_ONE_:
            isNoop = __TRACKER_RIGHTHAND == 1;
            break;
        default:
        case __TRACKER_NEVER:
            isNoop = false;
            break;
    }

    if (!isNoop) {

        TRACKER_GLOBAL_HOOK_((uintptr_t)&this->TrackedValue);
        return (this->TrackedValue TRACKER_OPERATOR_ __TRACKER_RIGHTHAND);
    } else {
        return this->TrackedValue;
    }
}
