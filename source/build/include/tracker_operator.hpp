
template <typename TrackedType>
template <typename RightHandType>
inline TrackedType TRACKER_NAME_<TrackedType>::operator TRACKER_OPERATOR_ (RightHandType rightHand)
{
    bool isNoop = false;

    switch (TRACKER_NOOP_)
    {
        case TRACKER_NOOP_RIGHTHAND_EQUAL_: isNoop = (this->TrackedValue == (TrackedType)rightHand); break;
        case TRACKER_NOOP_RIGHTHAND_ZERO_: isNoop  = (rightHand == 0); break;
        case TRACKER_NOOP_RIGHTHAND_ONE_: isNoop   = (rightHand == 1);
            fallthrough__;
        // case __TRACKER_NEVER:
        default:
            break;
    }

    if (!isNoop)
    {
        TRACKER_GLOBAL_HOOK_((uintptr_t) & this->TrackedValue);
        return this->TrackedValue TRACKER_OPERATOR_ (TrackedType)rightHand;
    }
    else return this->TrackedValue;
}
