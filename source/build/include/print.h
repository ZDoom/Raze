
#pragma once

#ifndef print_h_
#define print_h_

#ifdef HAVE_CXX11_HEADERS

template <typename T>
struct binwrap
{
    T data;
};
template <typename T>
static FORCE_INLINE constexpr binwrap<T> bin(T x)
{
    return binwrap<T>{x};
}
template <typename T>
struct octwrap
{
    T data;
};
template <typename T>
static FORCE_INLINE constexpr octwrap<T> oct(T x)
{
    return octwrap<T>{x};
}
template <typename T>
struct hexwrap
{
    T data;
};
template <typename T>
static FORCE_INLINE constexpr hexwrap<T> hex(T x)
{
    return hexwrap<T>{x};
}
template <typename T>
struct HEXwrap
{
    T data;
};
template <typename T>
static FORCE_INLINE constexpr HEXwrap<T> HEX(T x)
{
    return HEXwrap<T>{x};
}

FORCE_INLINE constexpr size_t buildprintpiece(void)
{
    return 0;
}

template<typename T>
enable_if_t<is_integral<T>::value, size_t> buildprintpiece(T x)
{
    // log_10(2^x) = x * log(2)/log(10) ~= x * 0.30103
    size_t constexpr numdigits = (sizeof(T) * CHAR_BIT + 1) / 3;
    char str[numdigits + 2]; // +2 for '-', '\0'

    char * strptr = str;

    size_t totalChars;
    if (x < 0)
    {
        size_t numChars = logbasenegative<10>(x);
        totalChars = numChars+1;
        *strptr++ = '-';
        strptr[numChars] = '\0';
        do
        {
            DivResult<T> const qr = divrhs<10>(x);
            x = qr.q;
            strptr[numChars-1] = '0' - (char)qr.r;
        }
        while (--numChars);
    }
    else
    {
        size_t numChars = logbase<10>(x);
        totalChars = numChars;
        strptr[numChars] = '\0';
        do
        {
            DivResult<T> const qr = divrhs<10>(x);
            x = qr.q;
            strptr[numChars-1] = '0' + (char)qr.r;
        }
        while (--numChars);
    }

    initputs(str);
    return totalChars;
}

template<typename T>
enable_if_t<is_integral<T>::value, size_t> buildprintpiece(binwrap<T> x)
{
    make_unsigned_t<T> const data = x.data;

    int constexpr numChars = sizeof(x)*CHAR_BIT;
    char str[numChars+1];
    str[numChars] = '\0';

    for (int p = 0; p < numChars; ++p)
        str[numChars - 1 - p] = '0' + (char)((data >> p) & 1);

    initputs(str);
    return numChars;
}

template<typename T>
enable_if_t<is_integral<T>::value, size_t> buildprintpiece(octwrap<T> x)
{
    make_unsigned_t<T> const data = x.data;

    int constexpr numChars = (sizeof(x)*CHAR_BIT + 2) / 3;
    char str[numChars+1];
    str[numChars] = '\0';

    for (int p = 0; p < numChars; ++p)
        str[numChars - 1 - p] = '0' + (char)((data >> (p*3)) & 7);

    initputs(str);
    return numChars;
}

template<typename T>
enable_if_t<is_integral<T>::value, size_t> buildprintpiece(hexwrap<T> x)
{
    static char const hexletters[] = "0123456789abcdef";

    make_unsigned_t<T> const data = x.data;

    int constexpr numChars = (sizeof(x)*CHAR_BIT + 3) >> 2;
    char str[numChars+1];
    str[numChars] = '\0';

    for (int p = 0; p < numChars; ++p)
        str[numChars - 1 - p] = hexletters[(int)((data >> (p<<2)) & 0xF)];

    initputs(str);
    return numChars;
}

template<typename T>
enable_if_t<is_integral<T>::value, size_t> buildprintpiece(HEXwrap<T> x)
{
    static char const HEXletters[] = "0123456789ABCDEF";

    make_unsigned_t<T> const data = x.data;

    int constexpr numChars = (sizeof(x)*CHAR_BIT + 3) >> 2;
    char str[numChars+1];
    str[numChars] = '\0';

    for (int p = 0; p < numChars; ++p)
        str[numChars - 1 - p] = HEXletters[(int)((data >> (p<<2)) & 0xF)];

    initputs(str);
    return numChars;
}

template <typename T>
FORCE_INLINE size_t buildprintpiece(const T * x)
{
    return buildprintpiece(hex((uintptr_t)x));
}

FORCE_INLINE size_t buildprintpiece(char const *str)
{
    initputs(str);
    return strlen(str);
}

template<typename T>
static FORCE_INLINE size_t buildprint(T first)
{
    return buildprintpiece(first);
}

template<typename T, typename... Args>
size_t buildprint(T first, Args... args)
{
    size_t const len = buildprintpiece(first);
    return len + buildprint(args...);
}

// this file is incomplete. a fuller implementation exists but has not been completed and debugged.

#endif

#endif // print_h_
