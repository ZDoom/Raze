template<typename T>
class TRACKER_NAME__
{
    public:
        T value;

        inline T *operator&()
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return &this->value;
        }

        inline T operator++()
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return ++this->value;
        }

        inline T operator++(int)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value++;
        }

        inline T operator--()
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return --this->value;
        }

        inline T operator--(int)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value--;
        }

        template <typename U> inline T operator=(U operand)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value = (T)operand;
        }

        template <typename U> inline T operator+=(U operand)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value += (T)operand;
        }

        template <typename U> inline T operator-=(U operand)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value -= (T)operand;
        }

        template <typename U> inline T operator*=(U operand)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value *= (T)operand;
        }

        template <typename U> inline T operator/=(U operand)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value /= (T)operand;
        }

        template <typename U> inline T operator|=(U operand)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value |= (T)operand;
        }

        template <typename U> inline T operator&=(U operand)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value &= (T)operand;
        }

        template <typename U> inline T operator^=(U operand)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value ^= (T)operand;
        }

        template <typename U> inline T operator<<=(U operand)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value <<= (T)operand;
        }

        template <typename U> inline T operator>>=(U operand)
        {
            TRACKER_HOOK_((intptr_t) & this->value);
            return this->value >>= (T)operand;
        }

        inline operator T() const { return this->value; }

        inline T cast() const { return this->value; }
};

template <typename T> struct is_signed<TRACKER_NAME__<T>>
{
    static constexpr bool value = std::is_signed<T>::value;
};
template <typename T> struct is_unsigned<TRACKER_NAME__<T>>
{
    static constexpr bool value = std::is_unsigned<T>::value;
};
