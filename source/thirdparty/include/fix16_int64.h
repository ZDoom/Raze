#ifndef __libfixmath_int64_h__
#define __libfixmath_int64_h__

#include <stdint.h>

static inline constexpr  int64_t int64_const(int32_t hi, uint32_t lo) { return (((int64_t)hi << 32) | lo); }
static inline constexpr  int64_t int64_from_int32(int32_t x) { return (int64_t)x; }
static inline constexpr  int32_t int64_hi(int64_t x) { return (x >> 32); }
static inline constexpr uint32_t int64_lo(int64_t x) { return (x & ((1ULL << 32) - 1)); }

static inline constexpr int64_t int64_add(int64_t x, int64_t y)   { return (x + y);  }
static inline constexpr int64_t int64_neg(int64_t x)              { return (-x);     }
static inline constexpr int64_t int64_sub(int64_t x, int64_t y)   { return (x - y);  }
static inline constexpr int64_t int64_shift(int64_t x, int8_t y)  { return (y < 0 ? (x >> -y) : (x << y)); }

static inline constexpr int64_t int64_mul_i32_i32(int32_t x, int32_t y) { return (x * y);  }
static inline constexpr int64_t int64_mul_i64_i32(int64_t x, int32_t y) { return (x * y);  }

static inline constexpr int64_t int64_div_i64_i32(int64_t x, int32_t y) { return (x / y);  }

static inline constexpr int int64_cmp_eq(int64_t x, int64_t y) { return (x == y); }
static inline constexpr int int64_cmp_ne(int64_t x, int64_t y) { return (x != y); }
static inline constexpr int int64_cmp_gt(int64_t x, int64_t y) { return (x >  y); }
static inline constexpr int int64_cmp_ge(int64_t x, int64_t y) { return (x >= y); }
static inline constexpr int int64_cmp_lt(int64_t x, int64_t y) { return (x <  y); }
static inline constexpr int int64_cmp_le(int64_t x, int64_t y) { return (x <= y); }

#endif
