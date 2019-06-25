template<typename T>
class TRACKER_NAME__
{
    public:
        T value;

        inline T *operator&()
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return &this->value;
        }

        inline T operator++()
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return ++this->value;
        }

        inline T operator++(int)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value++;
        }

        inline T operator--()
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return --this->value;
        }

        inline T operator--(int)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value--;
        }

        template <typename Tt> inline T operator=(Tt operand)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value = (T)operand;
        }

        template <typename Tt> inline T operator+=(Tt operand)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value += (T)operand;
        }

        template <typename Tt> inline T operator-=(Tt operand)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value -= (T)operand;
        }

        template <typename Tt> inline T operator*=(Tt operand)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value *= (T)operand;
        }

        template <typename Tt> inline T operator/=(Tt operand)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value /= (T)operand;
        }

        template <typename Tt> inline T operator|=(Tt operand)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value |= (T)operand;
        }

        template <typename Tt> inline T operator&=(Tt operand)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value &= (T)operand;
        }

        template <typename Tt> inline T operator^=(Tt operand)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value ^= (T)operand;
        }

        template <typename Tt> inline T operator<<=(Tt operand)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value <<= (T)operand;
        }

        template <typename Tt> inline T operator>>=(Tt operand)
        {
            TRACKER_HOOK_((uintptr_t) & this->value);
            return this->value >>= (T)operand;
        }

        inline operator T() const { return this->value; }

        inline T cast() const { return this->value; }

        struct is_signed
        {
            static constexpr bool value = std::is_signed<T>::value;
        };
        struct is_unsigned
        {
            static constexpr bool value = std::is_unsigned<T>::value;
        };
};
