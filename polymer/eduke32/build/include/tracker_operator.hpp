
template<typename TrackedType>
TrackedType __TRACKER_NAME<TrackedType>::operator __TRACKER_OPERATOR (__TRACKER_RIGHTHAND_TYPE)
{
    bool isNoop;

    switch (__TRACKER_NOOP) {
        case __TRACKER_NOOP_RIGHTHAND_EQUAL:
            isNoop = this->TrackedValue == __TRACKER_RIGHTHAND;
            break;
        case __TRACKER_NOOP_RIGHTHAND_ZERO:
            isNoop = __TRACKER_RIGHTHAND == 0;
            break;
        default:
        case __TRACKER_NEVER:
            isNoop = false;
            break;
    }

    if (!isNoop) {

        // hook here
        int TrackedAddress = (int)&this->TrackedValue;
        TrackedAddress += __TRACKER_GLOBAL_OFFSET;
        *((TrackedType*)TrackedAddress) = 1;
        return (this->TrackedValue __TRACKER_OPERATOR __TRACKER_RIGHTHAND);
    }
}